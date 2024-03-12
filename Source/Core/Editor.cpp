#include "Precomp.h"
#include "Core/Editor.h"
#include "Core/Device.h"

#include "Core/AssetManager.h"
#include "Core/VirtualMachine.h"
#include "Core/FileIO.h"
#include "Assets/Asset.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Meta/MetaTools.h"
#include "Meta/MetaProps.h"
#include "Utilities/DebugRenderer.h"
#include "EditorSystems/AssetEditorSystems/AssetEditorSystem.h"
#include "Containers/view_istream.h"
#include "GSON/GSONBinary.h"

void Engine::Editor::PostConstruct()
{
	const MetaType* const editorSystemType = MetaManager::Get().TryGetType<EditorSystem>();

	if (editorSystemType == nullptr)
	{
		LOG(LogEditor, Error, "EditorSystem was not reflected");
		return;
	}

	const std::filesystem::path pathToSavedState = GetFileLocationOfSystemState("Editor");

	std::ifstream editorSavedState{ pathToSavedState };

	std::function<void(const MetaType&)> recursivelyStart = [this, &recursivelyStart](const MetaType& type)
		{
			if (type.GetProperties().Has(Props::sEditorSystemDefaultOpenTag))
			{
				if (type.IsDefaultConstructible())
				{
					TryAddSystemInternal(type);
				}
				else
				{
					LOG(LogEditor, Error, "Could not {}, since type {} is not default constructible",
						Props::sEditorSystemDefaultOpenTag,
						type.GetName());
				}
			}

			for (const MetaType& derived : type.GetDirectDerivedClasses())
			{
				recursivelyStart(derived);
			}
		};

	if (!editorSavedState.is_open())
	{
		recursivelyStart(*editorSystemType);
		return;
	}

	uint32 savedDataVersion{};
	editorSavedState >> savedDataVersion;

	if (savedDataVersion == 0)
	{
		recursivelyStart(*editorSystemType);
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

		if (typeOfSystem->IsDefaultConstructible())
		{
			TryAddSystemInternal(*typeOfSystem);
			continue;
		}

		std::string assetName = Internal::GetAssetNameBasedOnSystemName(systemName);
		std::optional<WeakAsset<>> asset = AssetManager::Get().TryGetWeakAsset<Asset>(assetName);

		if (asset.has_value())
		{
			TryOpenAssetForEdit(*asset);
		}
	}
}

Engine::Editor::~Editor()
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

void Engine::Editor::Tick(const float deltaTime)
{
	Device& device = Device::Get();
	device.NewFrame();

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

	device.EndFrame();

	FullFillRefreshRequests();
}

void Engine::Editor::FullFillRefreshRequests()
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
		SystemToRefresh(const MetaType& type) : mType(type) {}
		std::reference_wrapper<const MetaType> mType;

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

		SystemToRefresh& restoreInfo = restorationData.emplace_back(*systemType);

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

	if (combinedFlags & RefreshRequest::ReloadAssets)
	{
		VirtualMachine::Get().ClearCompilationResult();
		AssetManager::Get().UnloadAllUnusedAssets();
	}

	for (const RefreshRequest& refreshRequest : mRefreshRequests)
	{
		if (refreshRequest.mActionWhileUnloaded)
		{
			refreshRequest.mActionWhileUnloaded();
		}
	}

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
	LOG(LogEditor, Verbose, "Completed volatile actions");
}


void Engine::Editor::DestroySystem(const std::string_view systemName)
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

void Engine::Editor::Refresh(RefreshRequest&& request)
{
	mRefreshRequests.push_front(std::move(request));
}

void Engine::Editor::SaveAll()
{
	Refresh(RefreshRequest{ RefreshRequest::SaveAssetsToFile | RefreshRequest::Volatile });
}

Engine::EditorSystem* Engine::Editor::TryOpenAssetForEdit(const WeakAsset<Asset>& originalAsset)
{
	if (originalAsset.GetFileOfOrigin().has_value())
	{
		std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromFile(*originalAsset.GetFileOfOrigin());

		if (!loadInfo.has_value())
		{
			LOG(LogEditor, Warning, "Cannot open asset editor for {}, {} did not produce a valid AssetLoadInfo",
				originalAsset.GetName(),
				originalAsset.GetFileOfOrigin()->string());
			return nullptr;
		}

		return TryOpenAssetForEdit(std::move(*loadInfo));
	}

	// We can still open assets that did not come from a file
	return TryOpenAssetForEdit(originalAsset.MakeShared()->Save());
}

bool Engine::Editor::IsThereAnEditorTypeForAssetType(TypeId assetTypeId) const
{
	return GetAssetKeyAssetEditorPairs().find(assetTypeId) != GetAssetKeyAssetEditorPairs().end();
}

template<typename T>
static CONSTEVAL Engine::TypeForm Test(T&&)
{
	return Engine::MakeTypeForm<T>();
}

Engine::EditorSystem* Engine::Editor::TryOpenAssetForEdit(AssetLoadInfo&& loadInfo)
{
	const MetaType& assetType = loadInfo.GetAssetClass();
	FuncResult asset = assetType.Construct(loadInfo);

	if (asset.HasError())
	{
		LOG(LogEditor, Error, "Cannot open asset editor, {} did not have a suitable constructor that takes an AssetLoadInfo& - {}",
			assetType.GetName(),
			asset.Error());
		return nullptr;
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

void Engine::Editor::DestroyRequestedSystems()
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

std::filesystem::path Engine::Editor::GetFileLocationOfSystemState(const std::string_view systemName)
{
	return FileIO::Get().GetPath(FileIO::Directory::Intermediate, std::string{ "EditorSystemStates/" }.append(systemName).append(".txt"));
}

void Engine::Editor::LoadSystemState(EditorSystem& system)
{
	const std::filesystem::path file = GetFileLocationOfSystemState(system.GetName());

	std::ifstream stream{ file, std::ifstream::binary };

	if (!stream.is_open()) // File doesnt exist
	{
		return;
	}

	system.LoadState(stream);
}

void Engine::Editor::SaveSystemState(const EditorSystem& system)
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

Engine::EditorSystem* Engine::Editor::TryAddSystemInternal(const TypeId typeId, SystemPtr<EditorSystem> system)
{
	const std::string_view systemName = system->GetName();

	if (TryGetSystemInternal(systemName) != nullptr)
	{
		if (std::find(mSystemsToDestroy.begin(), mSystemsToDestroy.end(), systemName) != mSystemsToDestroy.end())
		{
			LOG(LogEditor, Warning, "Failed to add system {}: There is already a system with this name. A request has been made to destroy this system already, but the existing system will be only destroyed only at the end of this frame.", systemName);
		}
		else
		{
			LOG(LogEditor, Warning, "Failed to add system {}: as there is already a system with this name. Destroy the existing system before adding this one.", systemName);
		}

		return nullptr;
	}

	EditorSystem* returnValue = system.get();
	mSystems.emplace_back(typeId, std::move(system));
	LoadSystemState(*returnValue);
	return returnValue;
}

Engine::EditorSystem* Engine::Editor::TryAddSystemInternal(const MetaType& type)
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

Engine::EditorSystem* Engine::Editor::TryGetSystemInternal(const std::string_view systemName)
{
	const auto it = std::find_if(mSystems.begin(), mSystems.end(),
	                             [systemName](const std::pair<TypeId, SystemPtr<EditorSystem>>& other)
	                             {
		                             return other.second->GetName() == systemName;
	                             });

	return it == mSystems.end() ? nullptr : it->second.get();
}

Engine::EditorSystem* Engine::Editor::TryGetSystemInternal(const TypeId typeId)
{
	const auto it = std::find_if(mSystems.begin(), mSystems.end(),
	                             [typeId](const std::pair<TypeId, SystemPtr<EditorSystem>>& other)
	                             {
		                             return other.first == typeId;
	                             });

	return it == mSystems.end() ? nullptr : it->second.get();
}

void Engine::Editor::DisplayMainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::SmallButton("RefreshAll"))
		{
			Refresh({ RefreshRequest::Volatile });
		}

		if (ImGui::SmallButton("Save all"))
		{
			SaveAll();
		}

		if (ImGui::BeginMenu("View"))
		{
			std::function<void(const MetaType&)> recursivelyDisplayAsOption = [this, &recursivelyDisplayAsOption](const MetaType& type)
				{
					if (type.IsDefaultConstructible())
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

		if (ImGui::BeginMenu("DebugDrawing"))
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
			if (ImGui::MenuItem("All", nullptr, flags & DebugCategory::All))
			{
				flags ^= DebugCategory::All;
			}

			DebugRenderer::SetDebugCategoryFlags(static_cast<DebugCategory::Enum>(flags));

			ImGui::EndMenu();
		}
	}
	ImGui::EndMainMenuBar();
}
