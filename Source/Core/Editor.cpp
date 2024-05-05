#include "Precomp.h"
#include "Core/Editor.h"
#include "Core/Device.h"

#include "Core/AssetManager.h"
#include "Core/VirtualMachine.h"
#include "Core/FileIO.h"
#include "Assets/Asset.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Meta/MetaTools.h"
#include "Meta/MetaProps.h"
#include "Rendering/DebugRenderer.h"
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"
#include "Utilities/view_istream.h"
#include "GSON/GSONBinary.h"

namespace
{
	void SetCustomTheme();
}

void CE::Editor::PostConstruct()
{
	if (Device::IsHeadless())
	{
		return;
	}

	SetCustomTheme();

	const MetaType* const editorSystemType = MetaManager::Get().TryGetType<EditorSystem>();

	if (editorSystemType == nullptr)
	{
		LOG(LogEditor, Error, "EditorSystem was not reflected");
		return;
	}

	const std::filesystem::path pathToSavedState = GetFileLocationOfSystemState("Editor");

	std::ifstream editorSavedState{ pathToSavedState };

	std::function<void(const MetaType&)> recursivelyStart = [this, &recursivelyStart, &editorSavedState](const MetaType& type)
		{
			if (type.GetProperties().Has(Props::sEditorSystemAlwaysOpenTag)
				|| (!editorSavedState.is_open() && type.GetProperties().Has(Props::sEditorSystemDefaultOpenTag)))
			{
				if (type.IsDefaultConstructible())
				{
					TryAddSystemInternal(type);
				}
				else
				{
					LOG(LogEditor, Error, "Could not add system, since type {} is not default constructible",
						type.GetName());
				}
			}

			for (const MetaType& derived : type.GetDirectDerivedClasses())
			{
				recursivelyStart(derived);
			}
		};

	recursivelyStart(*editorSystemType);

	if (!editorSavedState.is_open())
	{
		return;
	}

	uint32 savedDataVersion{};
	editorSavedState >> savedDataVersion;

	if (savedDataVersion == 0)
	{
		return;
	}

	// Currently not being used, but if we ever
	// change the format, we can support
	// backwards compatibility.
	(void)savedDataVersion;

	uint32 debugFlags{};
	editorSavedState >> debugFlags;
	DebugRenderer::SetDebugCategoryFlags(static_cast<DebugCategory::Enum>(debugFlags));

	while (editorSavedState)
	{
		std::string systemTypeName{};
		std::string systemName{};

		editorSavedState >> systemTypeName;
		editorSavedState >> systemName;

		const MetaType* const typeOfSystem = MetaManager::Get().TryGetType(systemTypeName);

		if (typeOfSystem == nullptr
			|| !typeOfSystem->IsDerivedFrom<EditorSystem>())
		{
			continue;
		}

		if (typeOfSystem->IsDefaultConstructible()
			&& !typeOfSystem->GetProperties().Has(Props::sEditorSystemAlwaysOpenTag))
		{
			TryAddSystemInternal(*typeOfSystem);
			continue;
		}

		std::string assetName = Internal::GetAssetNameBasedOnSystemName(systemName);
		WeakAssetHandle asset = AssetManager::Get().TryGetWeakAsset<Asset>(assetName);

		if (asset != nullptr)
		{
			TryOpenAssetForEdit(asset);
		}
	}
}

CE::Editor::~Editor()
{
	DestroyRequestedSystems();

	const std::filesystem::path pathToSavedState = GetFileLocationOfSystemState("Editor");
	create_directories(pathToSavedState.parent_path());

	std::ofstream editorSavedState{ pathToSavedState };

	if (!editorSavedState.is_open())
	{
		LOG(LogEditor, Warning, "Could not save editor state, {} could not be opened", pathToSavedState.string());
	}

	static constexpr uint32 savedDataVersion = 1;
	editorSavedState << savedDataVersion << '\n';

	const uint32 flags = DebugRenderer::GetDebugCategoryFlags();
	editorSavedState << flags << '\n';

	// Give each system a chance to save their state
	for (const auto& [typeId, system] : mSystems)
	{
		if (editorSavedState.is_open())
		{
			const MetaType* const systemType = MetaManager::Get().TryGetType(typeId);

			if (systemType != nullptr)
			{
				editorSavedState << systemType->GetName() << '\n' << system->GetName() << '\n';
			}
		}

		DestroySystem(system->GetName());
	}

	DestroyRequestedSystems();
}

void CE::Editor::Tick(const float deltaTime)
{
	DestroyRequestedSystems();
	DisplayMainMenuBar();

	ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	// We have to use the index, since more systems might be added
	// while we are iterating over them.
	for (uint32 i = 0; i < mSystems.size(); i++)
	{
		mSystems[i].second->Tick(deltaTime);
	}

	DestroyRequestedSystems();

	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)
		&& ImGui::IsKeyDown(ImGuiKey_LeftShift)
		&& ImGui::IsKeyPressed(ImGuiKey_S))
	{
		SaveAll();
	}
}

void CE::Editor::FullFillRefreshRequests()
{
	if (mRefreshRequests.empty())
	{
		return;
	}

	LOG(LogEditor, Verbose, "Commiting volatile actions");

	uint32 combinedFlags{};

	for (const RefreshRequest& refreshRequest : mRefreshRequests)
	{
		combinedFlags |= refreshRequest.mFlags;
	}

	if (combinedFlags & RefreshRequest::SaveAssetsToFile)
	{
		LOG(LogEditor, Message, "Saving all assets");
	}

	std::forward_list<std::string> namesOfSystemsToRefresh{};

	if (combinedFlags & RefreshRequest::ReloadOtherSystems)
	{
		for (auto& [typeId, system] : mSystems)
		{
			namesOfSystemsToRefresh.push_front(system->GetName());
		}
	}
	else
	{
		for (const RefreshRequest& refreshRequest : mRefreshRequests)
		{
			namesOfSystemsToRefresh.push_front(refreshRequest.mNameOfSystemToRefresh);
		}
	}

	struct SystemToRefresh
	{
		SystemToRefresh(const MetaType& type, std::string nameOfSystem) :
			mType(type),
			mNameOfSystem(std::move(nameOfSystem))
		{}
		std::reference_wrapper<const MetaType> mType;
		std::string mNameOfSystem{};

		// Empty if this was not an asset editor,
		std::optional<AssetEditorSystemInterface::MementoStack> mAssetEditorRestoreData{};
	};

	std::vector<SystemToRefresh> restorationData{};

	for (const std::string& nameOfSystem : namesOfSystemsToRefresh)
	{
		auto it = std::find_if(mSystems.begin(), mSystems.end(),
			[&nameOfSystem](const std::pair<TypeId, SystemPtr<EditorSystem>>& other)
			{
				return other.second->GetName() == nameOfSystem;
			});

		if (it == mSystems.end())
		{
			continue;
		}

		auto& [typeId, system] = *it;

		const MetaType* const systemType = MetaManager::Get().TryGetType(typeId);

		if (systemType == nullptr)
		{
			LOG(LogEditor, Verbose, R"(System {} was not reflected and cannot be
 offloaded while the engine refreshes. May lead to crashes, assets not updating, 
or other unexpected behaviour. In these systems, do not hold onto any references 
or pointers to objects not owned by the system itself, as there is no guarantee 
that they will still be valid after the engine finishes refreshing.)",
system->GetName());
			continue;
		}

		SystemToRefresh& restoreInfo = restorationData.emplace_back(*systemType, nameOfSystem);

		std::ostringstream savedStateStream{};
		system->SaveState(savedStateStream);

		if (AssetEditorSystemInterface* assetEditor = dynamic_cast<AssetEditorSystemInterface*>(system.get());
			assetEditor != nullptr)
		{
			if (combinedFlags & RefreshRequest::SaveAssetsToFile)
			{
				assetEditor->SaveToFile();
			}

			restoreInfo.mAssetEditorRestoreData = assetEditor->ExtractMementoStack();
		}

		DestroySystem(system->GetName());
	}

	DestroyRequestedSystems();

	struct AssetThatWasNotOffloaded
	{
		std::string mName{};
		std::filesystem::path mFile{};
	};
	std::vector<AssetThatWasNotOffloaded> nonOffloadedAssets{};

	if (combinedFlags & RefreshRequest::ReloadAssets)
	{
		VirtualMachine::Get().ClearCompilationResult();

		// We have a custom garbage collect implementation here,
		// that only offloads assets if they might have dependencies
		// on other assets. This prevents having to offload hundreds
		// of textures and meshes, only to reload them a second later!
		// We do still check to see if the file was written to, in which
		// case we still offload it.

		bool wereAnyUnloaded;

		do
		{
			wereAnyUnloaded = false;
			for (WeakAssetHandle<> asset : AssetManager::Get().GetAllAssets())
			{
				if (!asset.IsLoaded()
					|| asset.GetMetaData().GetClass().GetProperties().Has(Props::sCannotReferenceOtherAssetsTag)
					|| !asset.GetFileOfOrigin().has_value()) // This asset was generated at runtime; if we unload it, we won't be able to load if back in again. 
				{
					continue;
				}

				asset.Unload();
				wereAnyUnloaded = true;
			}
		} while (wereAnyUnloaded);

		for (WeakAssetHandle<> asset : AssetManager::Get().GetAllAssets())
		{
			if (!asset.IsLoaded())
			{
				continue;
			}

			if (asset.GetMetaData().GetClass().GetProperties().Has(Props::sCannotReferenceOtherAssetsTag))
			{
				if (asset.GetFileOfOrigin().has_value())
				{
					nonOffloadedAssets.emplace_back(
						AssetThatWasNotOffloaded{
							asset.GetMetaData().GetName(),
							*asset.GetFileOfOrigin()
						});
				}
			}
			else
			{
				LOG(LogEditor, Warning, "We are refresing, but {} was still loaded into memory. Might lead to a crash", asset.GetMetaData().GetName());
			}
		}
	}

	// Iterate by index because more requests might be added
	for (size_t i = 0; i < mRefreshRequests.size(); i++)
	{
		RefreshRequest& request = mRefreshRequests[i];
		if (request.mActionWhileUnloaded)
		{
			request.mActionWhileUnloaded();
		}
	}

	// Only works on windows!
#ifdef PLATFORM_WINDOWS
	static constexpr auto toSysClock =
		[](std::filesystem::file_time_type fileTimePoint)
		{
			using namespace std::literals;
			return std::chrono::system_clock::time_point{ fileTimePoint.time_since_epoch() - 3'234'576h };
		};
#else
	static_assert(false, "Implementation needed for this platform");
#endif //

	// While these assets have no dependencies on other assets,
	// it's still possible that their file was modified. If this
	// is the case, we do offload the asset. If someone needs the
	// asset again, if will be loaded with the updated changes.
	for (const AssetThatWasNotOffloaded& nonOffloadedAsset : nonOffloadedAssets)
	{
		WeakAssetHandle<> asset = AssetManager::Get().TryGetWeakAsset(nonOffloadedAsset.mName);

		if (asset == nullptr)
		{
			continue;
		}

		if (!TRY_CATCH_LOG(
			if (!asset.GetFileOfOrigin().has_value() 
				|| *asset.GetFileOfOrigin() != nonOffloadedAsset.mFile
				|| !std::filesystem::exists(nonOffloadedAsset.mFile)
				|| toSysClock(std::filesystem::last_write_time(nonOffloadedAsset.mFile)) >= mTimeOfLastRefresh)
			{

				if (asset != nullptr)
				{
					asset.Unload();
				}
			}))
		{
			asset.Unload();
		}
	}

	mTimeOfLastRefresh = std::chrono::system_clock::now();

	if (combinedFlags & RefreshRequest::ReloadAssets)
	{
		VirtualMachine::Get().Recompile();
	}

	// Restore the engine to it's state before saving
	for (SystemToRefresh& restoreData : restorationData)
	{
		EditorSystem* system{};
		if (restoreData.mAssetEditorRestoreData.has_value())
		{
			AssetEditorSystemInterface::MementoStack& stack = *restoreData.mAssetEditorRestoreData;

			const AssetEditorSystemInterface::MementoAction* topAction = stack.PeekTop();
			ASSERT(topAction != nullptr);

			std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromStream(std::make_unique<view_istream>(topAction->mState));

			if (!loadInfo.has_value())
			{
				LOG(LogEditor, Error, "Failed to restore to asset editor, loadInfo was invalid");
				continue;
			}
			system = TryOpenAssetForEdit(std::move(*loadInfo));

			// Check if our asset was renamed.
			// The assets in our do-undo stack will all have the wrong name.
			// So if the names don't match discard the do-undo stack
			if (system != nullptr
				&& system->GetName() == restoreData.mNameOfSystem)
			{
				AssetEditorSystemInterface* systemAsAssetEditor = dynamic_cast<AssetEditorSystemInterface*>(system);

				if (systemAsAssetEditor != nullptr)
				{
					systemAsAssetEditor->SetMementoStack(std::move(stack));
				}
				else
				{
					LOG(LogEditor, Error, "Failed to restore do-undo stack, system was not an asset editor");
				}
			}
		}
		else
		{
			system = TryAddSystemInternal(restoreData.mType);
		}

		if (system == nullptr)
		{
			LOG(LogEditor, Error, "Something went wrong while refreshing the engine, and system of type {} could not be restored",
				restoreData.mType.get().GetName());
		}
	}

	mRefreshRequests.clear();
}


void CE::Editor::DestroySystem(const std::string_view systemName)
{
	if (std::find(mSystemsToDestroy.begin(), mSystemsToDestroy.end(), systemName) != mSystemsToDestroy.end())
	{
		LOG(LogEditor, Warning, "Attempted to remove window group {} more than once", systemName);
		return;
	}

	if (TryGetSystem(systemName) == nullptr)
	{
		LOG(LogEditor, Warning, "Attempted to remove window group {}, but there is no window with this name", systemName);
		return;
	}

	mSystemsToDestroy.emplace_front(systemName);
}

void CE::Editor::Refresh(RefreshRequest&& request)
{
	mRefreshRequests.emplace_back(std::move(request));
}

void CE::Editor::SaveAll()
{
	Refresh(RefreshRequest{ RefreshRequest::SaveAssetsToFile | RefreshRequest::Volatile });
}

CE::EditorSystem* CE::Editor::TryOpenAssetForEdit(const WeakAssetHandle<>& originalAsset)
{
	if (originalAsset == nullptr)
	{
		return nullptr;
	}

	if (originalAsset.GetFileOfOrigin().has_value())
	{
		std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromFile(*originalAsset.GetFileOfOrigin());

		if (!loadInfo.has_value())
		{
			LOG(LogEditor, Warning, "Cannot open asset editor for {}, {} did not produce a valid AssetLoadInfo",
				originalAsset.GetMetaData().GetName(),
				originalAsset.GetFileOfOrigin()->string());
			return nullptr;
		}

		return TryOpenAssetForEdit(std::move(*loadInfo));
	}

	// We can still open assets that did not come from a file
	return TryOpenAssetForEdit(AssetHandle<>{ originalAsset }->Save());
}

bool CE::Editor::IsThereAnEditorTypeForAssetType(TypeId assetTypeId) const
{
	return GetAssetKeyAssetEditorPairs().find(assetTypeId) != GetAssetKeyAssetEditorPairs().end();
}

CE::EditorSystem* CE::Editor::TryOpenAssetForEdit(AssetLoadInfo&& loadInfo)
{
	const MetaType& assetType = loadInfo.GetMetaData().GetClass();
	FuncResult asset = assetType.Construct(loadInfo);

	if (asset.HasError())
	{
		LOG(LogEditor, Error, "Cannot open asset editor, {} did not have a suitable constructor that takes an AssetLoadInfo& - {}",
			assetType.GetName(),
			asset.Error());
		return nullptr;
	}

	AssetHandle originalAsset = AssetManager::Get().TryGetAsset(loadInfo.GetMetaData().GetName());

	if (originalAsset == nullptr)
	{
		LOG(LogEditor, Message, "Cannot open asset editor, {} was deleted",
			loadInfo.GetMetaData().GetName());
		return nullptr;
	}

	// The name of our asset editor should match that of the renamed asset
	if (originalAsset.GetMetaData().GetName() != loadInfo.GetMetaData().GetName())
	{
		asset.GetReturnValue().As<Asset>()->SetName(originalAsset.GetMetaData().GetName());
	}

	const std::unordered_map<TypeId, TypeId>& assetToEditorMap = GetAssetKeyAssetEditorPairs();

	const auto it = assetToEditorMap.find(assetType.GetTypeId());

	if (it == assetToEditorMap.end())
	{
		LOG(LogEditor, Message, "No asset editor for assets of type {}", assetType.GetName());
		return nullptr;
	}

	const TypeId baseTypeIdOfAssetEditor = it->second;
	const MetaType* const baseOfAssetEditor = MetaManager::Get().TryGetType(baseTypeIdOfAssetEditor);

	if (baseOfAssetEditor == nullptr)
	{
		LOG(LogEditor, Warning, "AssetEditorSystem<{}> was not reflected", assetType.GetName());
		return nullptr;
	}

	// Typically we see something like this:
	// class MaterialEditor : public AssetEditorSystem<Material>
	// So let's make sure we spawn an instance of MaterialEditor and not of AssetEditorSystem<Material>
	const size_t numOfDerived = baseOfAssetEditor->GetDirectDerivedClasses().size();

	if (numOfDerived == 0)
	{
		LOG(LogEditor, Warning, "The class deriving from AssetEditorSystem<{}> was not reflected", assetType.GetName());
		return nullptr;
	}

	if (numOfDerived > 1)
	{
		LOG(LogEditor, Warning, "More than one class derived from AssetEditorSystem<{}>, cannot determine which one to use",
			assetType.GetName());
		return nullptr;
	}

	const MetaType& assetEditorType = baseOfAssetEditor->GetDirectDerivedClasses()[0];
	FuncResult createdEditor = assetEditorType.Construct(std::move(asset.GetReturnValue()));

	if (createdEditor.HasError())
	{
		LOG(LogEditor, Error, "Asset editor could not be constructed by moving the appropriate asset in - {}", createdEditor.Error());
		return nullptr;
	}

	SystemPtr<EditorSystem> uniquePtr = MakeUnique<EditorSystem>(std::move(createdEditor.GetReturnValue()));
	ASSERT(uniquePtr != nullptr);

	return TryAddSystemInternal(assetEditorType.GetTypeId(), std::move(uniquePtr));
}

void CE::Editor::DestroyRequestedSystems()
{
	for (const std::string& name : mSystemsToDestroy)
	{
		auto it = std::find_if(mSystems.begin(), mSystems.end(),

			[&name](const std::pair<TypeId, SystemPtr<EditorSystem>>& other)
			{
				return other.second->GetName() == name;
			});

		if (it == mSystems.end())
		{
			LOG(LogEditor, Verbose, "Trying to remove {}, but that window no longer exists or is not registered", name);
			continue;
		}

		// Give the system a chance to save it's settings
		SaveSystemState(*it->second);
		
		*it = std::move(mSystems.back());
		mSystems.pop_back();
	}
	mSystemsToDestroy.clear();
}

std::filesystem::path CE::Editor::GetFileLocationOfSystemState(const std::string_view systemName)
{
	return FileIO::Get().GetPath(FileIO::Directory::Intermediate, std::string{ "EditorSystemStates/" }.append(systemName).append(".txt"));
}

void CE::Editor::LoadSystemState(EditorSystem& system)
{
	const std::filesystem::path file = GetFileLocationOfSystemState(system.GetName());

	std::ifstream stream{ file, std::ifstream::binary };

	if (!stream.is_open()) // File doesnt exist
	{
		return;
	}

	system.LoadState(stream);
}

void CE::Editor::SaveSystemState(const EditorSystem& system)
{
	const std::filesystem::path file = GetFileLocationOfSystemState(system.GetName());
	create_directories(file.parent_path());

	std::ofstream stream{ file, std::ofstream::binary };

	if (!stream.is_open())
	{
		LOG(LogEditor, Warning, "Could not save system state to {} - file could not be opened", file.string());
		return;
	}

	system.SaveState(stream);
}

CE::EditorSystem* CE::Editor::TryAddSystemInternal(const TypeId typeId, SystemPtr<EditorSystem> system)
{
	const std::string_view systemName = system->GetName();

	if (TryGetSystemInternal(systemName) != nullptr)
	{
		ImGui::SetWindowFocus(Internal::GetSystemNameBasedOnAssetName(system->GetName()).c_str());
		LOG(LogEditor, Verbose, "Failed to add system {}, there is already a system with this name. Brought focus to the existing system instead.", systemName);
		return nullptr;
	}

	EditorSystem* returnValue = system.get();
	mSystems.emplace_back(typeId, std::move(system));
	LoadSystemState(*returnValue);
	return returnValue;
}

CE::EditorSystem* CE::Editor::TryAddSystemInternal(const MetaType& type)
{
	FuncResult constructResult = type.Construct();

	if (constructResult.HasError())
	{
		LOG(LogEditor, Error, "Could unexpectedly not {} with type {}, constructing must have failed - {}",
			Props::sEditorSystemDefaultOpenTag,
			type.GetName(),
			constructResult.Error());
		return nullptr;
	}

	SystemPtr<EditorSystem> system = MakeUnique<EditorSystem>(std::move(constructResult.GetReturnValue()));
	return TryAddSystemInternal(type.GetTypeId(), std::move(system));
}

CE::EditorSystem* CE::Editor::TryGetSystemInternal(const std::string_view systemName)
{
	const auto it = std::find_if(mSystems.begin(), mSystems.end(),
	                             [systemName](const std::pair<TypeId, SystemPtr<EditorSystem>>& other)
	                             {
		                             return other.second->GetName() == systemName;
	                             });

	return it == mSystems.end() ? nullptr : it->second.get();
}

CE::EditorSystem* CE::Editor::TryGetSystemInternal(const TypeId typeId)
{
	const auto it = std::find_if(mSystems.begin(), mSystems.end(),
	                             [typeId](const std::pair<TypeId, SystemPtr<EditorSystem>>& other)
	                             {
		                             return other.first == typeId;
	                             });

	return it == mSystems.end() ? nullptr : it->second.get();
}

void CE::Editor::DisplayMainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::SmallButton(ICON_FA_REFRESH))
		{
			Refresh({ RefreshRequest::Volatile });
		}
		ImGui::SetItemTooltip("Refresh all open windows");

		if (ImGui::SmallButton(ICON_FA_FLOPPY_O))
		{
			SaveAll();
		}
		ImGui::SetItemTooltip("Save all open assets");

		if (ImGui::BeginMenu(ICON_FA_WINDOW_RESTORE))
		{
			std::function<void(const MetaType&)> recursivelyDisplayAsOption = [this, &recursivelyDisplayAsOption](const MetaType& type)
				{
					if (type.IsDefaultConstructible()
						&& !type.GetProperties().Has(Props::sEditorSystemAlwaysOpenTag))
					{
						const EditorSystem* existingSystem = TryGetSystemInternal(type.GetTypeId());
						bool selected = existingSystem != nullptr;
						if (ImGui::MenuItem(type.GetName().c_str(), nullptr, &selected))
						{
							if (existingSystem != nullptr)
							{
								DestroySystem(existingSystem->GetName());
							}
							else
							{
								TryAddSystemInternal(type);
							}
						}
					}

					for (const MetaType& derived : type.GetDirectDerivedClasses())
					{
						recursivelyDisplayAsOption(derived);
					}
				};

			const MetaType* const editorSystemType = MetaManager::Get().TryGetType<EditorSystem>();

			if (editorSystemType != nullptr)
			{
				recursivelyDisplayAsOption(*editorSystemType);
			}
			else
			{
				LOG(LogEditor, Error, "EditorSystem was not reflected");
			}

			ImGui::EndMenu();
		}
		else
		{
			ImGui::SetItemTooltip("Select which windows are open");
		}

		if (ImGui::BeginMenu(ICON_FA_EYE))
		{
			unsigned int flags = DebugRenderer::GetDebugCategoryFlags();

			// If we had static reflection in c++ we could just 
			// loop over the debug categories and performa enum_to_string operation..

			if (ImGui::MenuItem("General", nullptr, flags & DebugCategory::General))
			{
				flags ^= DebugCategory::General;
			}
			if (ImGui::MenuItem("Gameplay", nullptr, flags & DebugCategory::Gameplay))
			{
				flags ^= DebugCategory::Gameplay;
			}
			if (ImGui::MenuItem("Physics", nullptr, flags & DebugCategory::Physics))
			{
				flags ^= DebugCategory::Physics;
			}
			if (ImGui::MenuItem("Sound", nullptr, flags & DebugCategory::Sound))
			{
				flags ^= DebugCategory::Sound;
			}
			if (ImGui::MenuItem("Rendering", nullptr, flags & DebugCategory::Rendering))
			{
				flags ^= DebugCategory::Rendering;
			}
			if (ImGui::MenuItem("AINavigation", nullptr, flags & DebugCategory::AINavigation))
			{
				flags ^= DebugCategory::AINavigation;
			}
			if (ImGui::MenuItem("AIDecision", nullptr, flags & DebugCategory::AIDecision))
			{
				flags ^= DebugCategory::AIDecision;
			}
			if (ImGui::MenuItem("Editor", nullptr, flags & DebugCategory::Editor))
			{
				flags ^= DebugCategory::Editor;
			}
			if (ImGui::MenuItem("AccelStructs", nullptr, flags & DebugCategory::AccelStructs))
			{
				flags ^= DebugCategory::AccelStructs;
			}
			if (ImGui::MenuItem("Particles", nullptr, flags & DebugCategory::Particles))
			{
				flags ^= DebugCategory::Particles;
			}
			if (ImGui::MenuItem("TerrainHeight", nullptr, flags & DebugCategory::TerrainHeight))
			{
				flags ^= DebugCategory::TerrainHeight;
			}
			if (ImGui::MenuItem("All", nullptr, flags & DebugCategory::All))
			{
				flags ^= DebugCategory::All;
			}

			DebugRenderer::SetDebugCategoryFlags(static_cast<DebugCategory::Enum>(flags));

			ImGui::EndMenu();
		}
		else
		{
			ImGui::SetItemTooltip("Specify which debug categories to draw");
		}
	}
	ImGui::EndMainMenuBar();
}

namespace
{
	void SetCustomTheme()
	{
		ImGuiIO& io = ImGui::GetIO();
		const std::string fontPath = CE::FileIO::Get().GetPath(CE::FileIO::Directory::EngineAssets, "Fonts/Roboto-Regular.ttf");
		const std::string iconsPath = CE::FileIO::Get().GetPath(CE::FileIO::Directory::EngineAssets, "Fonts/fontawesome-webfont.ttf");

		ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);

		if (font == nullptr)
		{
			LOG(LogEditor, Warning, "Failed to load custom font {}. Using ImGui's default font instead", fontPath.c_str());
			io.Fonts->AddFontDefault();
		}
		else
		{
			io.FontDefault = font;
		}

		// Merge icons into default font
		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		io.Fonts->AddFontFromFileTTF(iconsPath.c_str(), 13.0f, &config, icon_ranges);

		ImGui::GetStyle().FrameRounding = 4.0f;
		ImGui::GetStyle().GrabRounding = 4.0f;

		static constexpr float brightness = .8f;
		static constexpr glm::vec4 accent = glm::vec4(0.14f * brightness, 0.51f * brightness, 0.50f * brightness, 1.00f);
		static constexpr glm::vec4 hovered = glm::vec4(0.18f * brightness, 0.66f * brightness, 0.63f * brightness, 1.00f);
		static constexpr glm::vec4 active = glm::vec4(0.22f * brightness, 0.77f * brightness, 0.74f * brightness, 1.00f);
		static constexpr glm::vec4 unfocused = glm::vec4(accent.x * .8f, accent.y * .8f, accent.z * .8f, accent.w);
		static constexpr glm::vec4 unfocusedActive = glm::vec4(accent.x * .9f, accent.y * .9f, accent.z * .9f, accent.w);

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);;
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
		colors[ImGuiCol_TitleBgActive] = glm::vec4(accent.x * .7f, accent.y * .7f, accent.z * .7f, accent.w);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
		colors[ImGuiCol_ScrollbarGrab] = accent;
		colors[ImGuiCol_ScrollbarGrabHovered] = hovered;
		colors[ImGuiCol_ScrollbarGrabActive] = active;
		colors[ImGuiCol_CheckMark] = active;
		colors[ImGuiCol_SliderGrab] = accent;
		colors[ImGuiCol_SliderGrabActive] = active;
		colors[ImGuiCol_Button] = accent;
		colors[ImGuiCol_ButtonHovered] = hovered;
		colors[ImGuiCol_ButtonActive] = active;
		colors[ImGuiCol_Header] = accent;
		colors[ImGuiCol_HeaderHovered] = hovered;
		colors[ImGuiCol_HeaderActive] = active;
		colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);;
		colors[ImGuiCol_SeparatorHovered] = hovered;
		colors[ImGuiCol_SeparatorActive] = active;
		colors[ImGuiCol_ResizeGrip] = accent;
		colors[ImGuiCol_ResizeGripHovered] = hovered;
		colors[ImGuiCol_ResizeGripActive] = active;
		colors[ImGuiCol_Tab] = accent;
		colors[ImGuiCol_TabHovered] = hovered;
		colors[ImGuiCol_TabActive] = active;
		colors[ImGuiCol_TabUnfocused] = unfocused;
		colors[ImGuiCol_TabUnfocusedActive] = unfocusedActive;
		colors[ImGuiCol_DockingPreview] = hovered;
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	}
}