#include "Precomp.h"
#include "EditorSystems/ContentBrowserEditorSystem.h"

#include <imgui/imgui_internal.h>

#include "Assets/Asset.h"
#include "Assets/Texture.h"
#include "Core/AssetManager.h"
#include "Core/Editor.h"
#include "Core/FileIO.h"
#include "Core/Input.h"
#include "EditorSystems/ImporterSystem.h"
#include "Utilities/Imgui/ImguiDragDrop.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Utilities/Search.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaTypeFilter.h"
#include "EditorSystems/ThumbnailEditorSystem.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Imgui/ImguiHelpers.h"

namespace
{
	constexpr std::string_view sEngineAssetsDisplayName = "EngineAssets";
	constexpr std::string_view sGameAssetsDisplayName = "GameAssets";

	std::string sPopUpFolderRelativeToRoot{};
	std::string sPopUpNewAssetName{};
	bool sPopUpIsEngineAsset{};
	const CE::MetaType* sPopUpAssetClass{};
}

CE::ContentBrowserEditorSystem::ContentBrowserEditorSystem() :
	EditorSystem("ContentBrowser")
{
	RequestUpdateToFolderGraph();
}

CE::ContentBrowserEditorSystem::~ContentBrowserEditorSystem()
{
	if (mPendingRootFolder.GetThread().WasLaunched())
	{
		mPendingRootFolder.GetThread().CancelOrDetach();
	}
}

void CE::ContentBrowserEditorSystem::Tick(const float)
{
	if (!Begin())
	{
		End();
		return;
	}

	if (mPendingRootFolder.IsReady())
	{
		mRootFolder = std::move(mPendingRootFolder.Get());

		// I didn't feel like making a move constructor,
		// plus this reassignment only needs to happen for
		// the rootfolder
		for (ContentFolder& child : mRootFolder.mChildren)
		{
			child.mParent = &mRootFolder;
		}

		mPendingRootFolder = {};
		mSelectedFolder = mRootFolder;
	}

	ImGui::Splitter(true, &mFolderHierarchyPanelWidthPercentage, &mContentPanelWidthPercentage);

	DisplayFolderHierarchyPanel();
	ImGui::SameLine();

	DisplayContentPanel();

	End();
}

void CE::ContentBrowserEditorSystem::RequestUpdateToFolderGraph()
{
	if (mPendingRootFolder.GetThread().WasLaunched())
	{
		mPendingRootFolder.GetThread().CancelOrDetach();
	}

	mPendingRootFolder = 
	{
		[]
		{
			std::unordered_map<std::filesystem::path, WeakAssetHandle<>> assetLookUp{};

			for (WeakAssetHandle<> asset : AssetManager::Get().GetAllAssets())
			{
				if (asset.GetFileOfOrigin().has_value())
				{
					assetLookUp.emplace(*asset.GetFileOfOrigin(), asset);
				}
			}

			// Create the root folder to hold the top-level directories and files
			ContentFolder rootFolder = { "", "All", nullptr };

			ContentFolder& engineFolder = rootFolder.mChildren.emplace_back(FileIO::Get().GetPath(FileIO::Directory::EngineAssets, ""), std::string{ sEngineAssetsDisplayName }, &rootFolder);
			ContentFolder& gameFolder = rootFolder.mChildren.emplace_back(FileIO::Get().GetPath(FileIO::Directory::GameAssets, ""), std::string{ sGameAssetsDisplayName }, &rootFolder);

			std::function<void(ContentFolder&)> parse = [&parse, &assetLookUp](ContentFolder& folder)
				{
					for (std::filesystem::directory_entry entry : std::filesystem::directory_iterator{ folder.mActualPath })
					{
						std::filesystem::path path = entry.path();;

						if (entry.is_directory())
						{
							parse(folder.mChildren.emplace_back(path, path.filename().string(), &folder));
							continue;
						}

						if (path.extension() != AssetManager::sAssetExtension)
						{
							continue;
						}

						auto it = assetLookUp.find(path);

						if (it == assetLookUp.end())
						{
							continue;
						}

						folder.mContent.emplace_back(it->second);
					}
				};
			parse(engineFolder);
			parse(gameFolder);

			return rootFolder;
		}
	};
}

void CE::ContentBrowserEditorSystem::DisplayFolderHierarchyPanel()
{
	if (ImGui::BeginChild("FolderHierarchy", { mFolderHierarchyPanelWidthPercentage, -2.0f }))
	{
		ImGui::SetItemTooltip("Create a new asset");
		ImGui::SameLine();
		Search::Begin();

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		DisplayFolder(mRootFolder);

		Search::End();
	}
	ImGui::EndChild();
}

void CE::ContentBrowserEditorSystem::DisplayContentPanel()
{
	if (!ImGui::BeginChild("ContentPanel", { mContentPanelWidthPercentage, -2.0f }))
	{
		ImGui::EndChild();
		return;
	}

	ThumbnailEditorSystem* thumbnailEditorSystem = Editor::Get().TryGetSystem<ThumbnailEditorSystem>();

	if (thumbnailEditorSystem == nullptr)
	{
		LOG(LogEditor, Error, "Could not display content, no thumbnail editor system!");
		ImGui::EndChild();
		return;
	}

	DisplayPathToCurrentlySelectedFolder(mSelectedFolder);
	ImGui::NewLine();

	static std::vector<std::reference_wrapper<ContentFolder>> foldersToDisplay{};
	static std::vector<WeakAssetHandle<>> assetsToDisplay{};

	ImGui::SameLine();
	Search::Begin(Search::DontCreateChildForContent);

	if (!Search::GetUserQuery().empty())
	{
		const auto addFolderRecursive = [](const auto& self, ContentFolder& node) -> void
			{
				for (ContentFolder& child : node.mChildren)
				{
					Search::AddItem(child.mFolderName,
						[&child](std::string_view)
						{
							foldersToDisplay.emplace_back(child);
							return false;
						});
				}

				for (const WeakAssetHandle<>& asset : node.mContent)
				{
					Search::AddItem(asset.GetMetaData().GetName(),
						[&asset](std::string_view)
						{
							assetsToDisplay.emplace_back(asset);
							return false;
						});
				}
				
				for (ContentFolder& child : node.mChildren)
				{
					self(self, child);
				}
			};
		addFolderRecursive(addFolderRecursive, mSelectedFolder);
	}
	else
	{
		foldersToDisplay.insert(foldersToDisplay.begin(), mSelectedFolder.get().mChildren.begin(), mSelectedFolder.get().mChildren.end());
		assetsToDisplay.insert(assetsToDisplay.begin(), mSelectedFolder.get().mContent.begin(), mSelectedFolder.get().mContent.end());
	}

	Search::End();

	ImGui::BeginTable("TableTest",
		std::max(static_cast<int>(ImGui::GetContentRegionAvail().x / (ThumbnailEditorSystem::sGeneratedThumbnailResolution.x + 10.0f)), 1),
		ImGuiTableColumnFlags_WidthFixed);

	for (ContentFolder& folder : foldersToDisplay)
	{
		DisplayItemInFolder(folder, *thumbnailEditorSystem);
	}

	for (const WeakAssetHandle<>& content : assetsToDisplay)
	{
		DisplayItemInFolder(content, *thumbnailEditorSystem);
	}

	foldersToDisplay.clear();
	assetsToDisplay.clear();

	ImGui::EndTable();
	ImGui::EndChild();
}

void CE::ContentBrowserEditorSystem::DisplayFolder(ContentFolder& folder)
{
	const bool displayAsTreeNode = !folder.mChildren.empty();

	Search::BeginCategory(folder.mFolderName,
		[&folder, this, displayAsTreeNode](std::string_view folderName) -> bool
		{
			bool isSelected = &mSelectedFolder.get() == &folder;

			auto recursiveCheckIfChildIsSelected = [this, &folder](const auto& self, const ContentFolder& folderToCheck) -> bool
				{
					if (&folder == &folderToCheck)
					{
						return true;
					}

					if (folderToCheck.mParent == nullptr)
					{
						return false;
					}
					return self(self, *folderToCheck.mParent);
				};

			const bool isOneOfChildrenSelected = recursiveCheckIfChildIsSelected(recursiveCheckIfChildIsSelected, mSelectedFolder);

			bool isOpen{};
			if (displayAsTreeNode)
			{
				if (isOneOfChildrenSelected)
				{
					ImGui::SetNextItemOpen(true);
				}

				isOpen = ImGui::TreeNode(Format("##{}", folderName).data());
				ImGui::SameLine();
			}
			else
			{
				// The three node arrow take up space
				// this makes sure that all folders are
				// aligned, regardless of whether they
				// have content
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 24.0f);
			}

			const ImVec2 selectableAreaSize = { ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() };

			if (isOneOfChildrenSelected
				&& !isSelected)
			{
				glm::vec4 col = ImGui::GetStyle().Colors[ImGuiCol_HeaderActive];
				col *= .7f;
				ImGui::RenderFrame(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + selectableAreaSize, ImColor{ col }, false, 0.0f);
			}

			if (ImGui::Selectable(Format("{} {}", isOpen ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, folderName).data(), &isSelected, 0, selectableAreaSize))
			{
				if (isSelected)
				{
					mSelectedFolder = folder;
				}
				else
				{
					mSelectedFolder = mRootFolder;
				}
			}

			if (&folder != &mRootFolder)
			{
				WeakAssetHandle receivedAsset = DragDrop::PeekAsset<Asset>();

				if (receivedAsset != nullptr
					&& receivedAsset.GetFileOfOrigin().has_value()
					&& DragDrop::AcceptAsset())
				{
					AssetManager::Get().MoveAsset(receivedAsset, folder.mActualPath / receivedAsset.GetFileOfOrigin()->filename());
				}
			}

			if (ImGui::IsItemClicked(1))
			{
				ImGui::OpenPopup(GetRightClickPopUpMenuName(GetName(folder)).data());
			}

			DisplayRightClickMenu(folder);

			return isOpen;
		});

	for (ContentFolder& child : folder.mChildren)
	{
		DisplayFolder(child);
	}

	Search::EndCategory(
		[displayAsTreeNode]
		{
			if (displayAsTreeNode)
			{
				ImGui::TreePop();
			}
		});
}

template <typename T>
void CE::ContentBrowserEditorSystem::DisplayItemInFolder(T& item, ThumbnailEditorSystem& thumbnailSystem)
{
	ImGui::TableNextColumn();

	auto name = GetName(item);
	ImGui::PushID(name.data(), name.data() + name.size());

	DisplayImage(item, thumbnailSystem);

	if (ImGui::IsItemClicked(1))
	{
		ImGui::OpenPopup(GetRightClickPopUpMenuName(name).data());
	}

	float textWidth = ImGui::CalcTextSize(name.data(), name.data() + name.size()).x;

	if (textWidth >= ThumbnailEditorSystem::sGeneratedThumbnailResolution.x)
	{
		const float newFontScale = std::max(ThumbnailEditorSystem::sGeneratedThumbnailResolution.x / textWidth, .75f);
		ImGui::SetWindowFontScale(newFontScale);
		textWidth *= newFontScale;
	}

	if (textWidth < ThumbnailEditorSystem::sGeneratedThumbnailResolution.x)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ThumbnailEditorSystem::sGeneratedThumbnailResolution.x - textWidth) * 0.55f);
	}

	ImGui::TextWrapped(name.data());
	ImGui::SetWindowFontScale(1.0f);

	DisplayRightClickMenu(item);

	ImGui::PopID();
}

void CE::ContentBrowserEditorSystem::OpenAsset(WeakAssetHandle<> asset)
{
	Editor::Get().TryOpenAssetForEdit(asset);
}

bool CE::ContentBrowserEditorSystem::DisplayNameUI(std::string& assetName)
{
	ShowInspectUI("Name", assetName);

	bool anyErrors = false;
	PushError();

	if (assetName.empty())
	{
		ImGui::TextUnformatted("Name is empty");
		anyErrors = true;
	}

	if (AssetManager::Get().TryGetWeakAsset(Name{ assetName }) != nullptr)
	{
		ImGui::Text("There is already an asset with the name %s", assetName.c_str());
		anyErrors = true;
	}
	PopError();
	return anyErrors;
}

CE::ContentBrowserEditorSystem::FilePathUIResult CE::ContentBrowserEditorSystem::DisplayFilepathUI(std::string& folderRelativeToRoot, bool& isEngineAsset,
	const std::string& assetName)
{
	ShowInspectUI("IsEngineAsset", sPopUpIsEngineAsset);
	ShowInspectUI("File location", folderRelativeToRoot);

	std::filesystem::path outputRootFolder = folderRelativeToRoot;

	while (outputRootFolder.has_parent_path())
	{
		outputRootFolder = outputRootFolder.parent_path();
	}

	const std::filesystem::path actualOutputFile =
		Format("{}{}{}{}{}",
			isEngineAsset ? FileIO::Get().GetPath(FileIO::Directory::EngineAssets, "") : FileIO::Get().GetPath(FileIO::Directory::GameAssets, ""),
			folderRelativeToRoot,
			folderRelativeToRoot.empty() ? "" : "/",
			assetName,
			AssetManager::sAssetExtension);

	// Gaslight our users
	const std::string outputFileToDisplay =
		Format("{}/{}{}{}{}",
			isEngineAsset ? sEngineAssetsDisplayName : sGameAssetsDisplayName,
			folderRelativeToRoot,
			folderRelativeToRoot.empty() ? "" : "/",
			assetName,
			AssetManager::sAssetExtension);

	bool anyErrors = false;
	PushError();

	if (std::filesystem::exists(actualOutputFile))
	{
		ImGui::Text("There is already a file at %s", outputFileToDisplay.c_str());
		anyErrors = true;
	}

	PopError();

	return { actualOutputFile, outputFileToDisplay, anyErrors };
}

void CE::ContentBrowserEditorSystem::PushError()
{
	ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.0f, 0.0f, 1.0f });
}

void CE::ContentBrowserEditorSystem::PopError()
{
	ImGui::PopStyleColor();
}

void CE::ContentBrowserEditorSystem::Reimport(const WeakAssetHandle<>& asset)
{
	if (!asset.GetMetaData().GetImporterInfo().has_value())
	{
		return;
	}

	ImporterSystem* importerSystem = Editor::Get().TryGetSystem<ImporterSystem>();

	if (importerSystem != nullptr)
	{
		importerSystem->Import(asset.GetMetaData().GetImporterInfo()->mImportedFile, "Requested by user");
	}
	else
	{
		LOG(LogEditor, Error, "Could not import file, importer system does not exist!");
	}
}

std::string_view CE::ContentBrowserEditorSystem::GetName(const WeakAssetHandle<>& asset)
{
	return asset.GetMetaData().GetName();
}

std::string_view CE::ContentBrowserEditorSystem::GetName(const ContentFolder& folder)
{
	return folder.mFolderName;
}

void CE::ContentBrowserEditorSystem::DisplayPathToCurrentlySelectedFolder(ContentFolder& folder)
{
	if (folder.mParent != nullptr)
	{
		DisplayPathToCurrentlySelectedFolder(*folder.mParent);
	}

	if (ImGui::Button(folder.mFolderName.data()))
	{
		mSelectedFolder = folder;
	}
	ImGui::SameLine();
	ImGui::TextUnformatted("/");
	ImGui::SameLine();
}

void CE::ContentBrowserEditorSystem::DisplayImage(const WeakAssetHandle<>& asset, ThumbnailEditorSystem& thumbnailSystem)
{
	if (Editor::Get().IsThereAnEditorTypeForAssetType(asset.GetMetaData().GetClass().GetTypeId()))
	{
		if (thumbnailSystem.DisplayImGuiImageButton(asset, thumbnailSystem.sGeneratedThumbnailResolution))
		{
			OpenAsset(asset);
		}
	}
	else
	{
		thumbnailSystem.DisplayImGuiImage(asset, thumbnailSystem.sGeneratedThumbnailResolution);
	}

	const std::string& assetName = asset.GetMetaData().GetName();

	DragDrop::SendAsset(assetName);

	if (ImGui::BeginItemTooltip())
	{
		ImGui::Text("Name: %s", assetName.c_str());
		ImGui::Text("Type: %s", asset.GetMetaData().GetClass().GetName().c_str());

		if (asset.GetFileOfOrigin().has_value())
		{
			ImGui::Text("File: %s", asset.GetFileOfOrigin()->string().c_str());
		}
		else
		{
			ImGui::Text("Generated at runtime");
		}

		if (const std::optional<AssetFileMetaData::ImporterInfo> importerInfo = asset.GetMetaData().GetImporterInfo();
			importerInfo.has_value())
		{
			ImGui::Text("Imported from: %s", importerInfo->mImportedFile.string().c_str());
		}

		ImGui::EndTooltip();
	}
}

void CE::ContentBrowserEditorSystem::DisplayImage(ContentFolder& folder, ThumbnailEditorSystem& thumbnailSystem)
{
	WeakAssetHandle<Texture> thumbnail = AssetManager::Get().TryGetWeakAsset<Texture>("T_FolderIcon");

	if (thumbnailSystem.DisplayImGuiImageButton(thumbnail, thumbnailSystem.sGeneratedThumbnailResolution))
	{
		mSelectedFolder = folder;
	}
}

std::string CE::ContentBrowserEditorSystem::GetRightClickPopUpMenuName(std::string_view itemName)
{
	return Format("RightClickPopup{}", itemName);
}

void CE::ContentBrowserEditorSystem::DisplayRightClickMenu(const WeakAssetHandle<>& asset)
{
	if (!ImGui::BeginPopup(GetRightClickPopUpMenuName(GetName(asset)).data()))
	{
		return;
	}

	if (ImGui::BeginMenu("Rename##Menu"))
	{
		const bool anyErrors = DisplayNameUI(sPopUpNewAssetName);

		ImGui::BeginDisabled(anyErrors);

		if (ImGui::Button("Rename"))
		{
			AssetManager::Get().RenameAsset(asset, sPopUpNewAssetName);
		}

		ImGui::EndDisabled();

		ImGui::EndMenu();
	}

	if (ImGui::MenuItem("Duplicate"))
	{
		const std::string copyName = StringFunctions::CreateUniqueName(asset.GetMetaData().GetName(),
			[](std::string_view name) -> bool
			{
				return AssetManager::Get().TryGetWeakAsset(name) == nullptr;
			});

		std::filesystem::path copyPath = asset.GetFileOfOrigin().value_or(FileIO::Get().GetPath(FileIO::Directory::GameAssets, copyName)).parent_path() / copyName;
		copyPath.replace_extension(AssetManager::sAssetExtension);

		WeakAssetHandle newAsset = AssetManager::Get().Duplicate(asset, copyPath);

		if (newAsset != nullptr)
		{
			Editor::Get().TryOpenAssetForEdit(newAsset);

			// Adding assets is not considered to be a volatile
			// action, our system will not restart, and we
			// have to explicitly state we want to recreate our
			// folder graph.
			RequestUpdateToFolderGraph();
		}
	}

	if (const std::optional<AssetFileMetaData::ImporterInfo>& importerInfo = asset.GetMetaData().GetImporterInfo();
		importerInfo.has_value()
		&& ImGui::MenuItem("Reimport"))
	{
		Reimport(asset);
	}

	if (ImGui::MenuItem("Delete", "Del")
		|| Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Delete))
	{
		AssetManager::Get().DeleteAsset(WeakAssetHandle{ asset });
	}

	ImGui::EndPopup();
}

void CE::ContentBrowserEditorSystem::DisplayRightClickMenu(const ContentFolder& folder)
{
	if (!ImGui::BeginPopup(GetRightClickPopUpMenuName(GetName(folder)).data()))
	{
		return;
	}

	if (&folder != &mRootFolder
		&& ImGui::MenuItem("New folder"))
	{
		const auto getPath = [&folder](std::string_view name)
			{
				return folder.mActualPath / name;
			};

		const std::string folderName = StringFunctions::CreateUniqueName("New folder",
			[&getPath](std::string_view name) -> bool
			{
				return !std::filesystem::exists(getPath(name));
			});

		std::filesystem::create_directory(getPath(folderName));
	}

	if (ImGui::MenuItem("Reimport"))
	{
		std::function<void(const ContentFolder&)> importRecursive = [&importRecursive](const ContentFolder& folder)
			{
				for (const WeakAssetHandle<>& asset : folder.mContent)
				{
					Reimport(asset);
				}

				for (const ContentFolder& child : folder.mChildren)
				{
					importRecursive(child);
				}
			};
		importRecursive(folder);
	}

	if (ImGui::MenuItem("Delete", "Del")
		|| Input::Get().WasKeyboardKeyPressed(Input::KeyboardKey::Delete))
	{
		std::function<void(const ContentFolder&)> deleteRecursive = [&deleteRecursive](const ContentFolder& folder)
			{
				for (const WeakAssetHandle<>& asset : folder.mContent)
				{
					AssetManager::Get().DeleteAsset(WeakAssetHandle{ asset });
				}

				for (const ContentFolder& child : folder.mChildren)
				{
					deleteRecursive(child);
				}
			};
		deleteRecursive(folder);

		if (&folder != &mRootFolder)
		{
			std::filesystem::remove(folder.mActualPath);
		}
	}

	ImGui::EndPopup();
}

void CE::ContentBrowserEditorSystem::ShowCreateNewMenu()
{
	//if (!ImGui::BeginPopup(sAssetCreatorImGuiId.data()))
//{
//	return;
//}

//struct OnlyAssetsThatCanBeEdited
//{
//	bool operator()(const MetaType& type) const
//	{
//		return Editor::Get().IsThereAnEditorTypeForAssetType(type.GetTypeId());
//	}
//};
//MetaTypeFilter<OnlyAssetsThatCanBeEdited> filter{ sPopUpAssetClass };
//ShowInspectUI("Type", filter);
//sPopUpAssetClass = filter.Get();

//bool anyErrors = false;
//anyErrors |= DisplayNameUI(sPopUpNewAssetName);

//const FilePathUIResult fileUIResult = DisplayFilepathUI(sPopUpFolderRelativeToRoot, sPopUpIsEngineAsset, sPopUpNewAssetName);

//anyErrors |= fileUIResult.mAnyErrors;

//PushError();

//if (sPopUpAssetClass == nullptr)
//{
//	ImGui::TextUnformatted("No type selected.");
//	anyErrors = true;
//}
//else if (sPopUpAssetClass->IsExactly(MakeTypeId<Asset>())
//	|| !sPopUpAssetClass->IsDerivedFrom<Asset>())
//{
//	ImGui::Text("%s is not a valid type.", sPopUpAssetClass->GetName().c_str());
//	anyErrors = true;
//}

//PopError();

//ImGui::BeginDisabled(anyErrors);

//if (ImGui::Button(Format("Save to {}", fileUIResult.mPathToShowUser.string()).c_str()))
//{
//	WeakAssetHandle newAsset = AssetManager::Get().NewAsset(*sPopUpAssetClass, fileUIResult.mActualFullPath);

//	if (newAsset != nullptr)
//	{
//		Editor::Get().TryOpenAssetForEdit(newAsset);

//		// Adding assets is not considered to be a volatile
//		// action, our system will not restart, and we
//		// have to explicitly state we want to recreate our
//		// folder graph.
//		mRootFolder = RequestUpdateToFolderGraph();
//	}
//}

//ImGui::EndDisabled();
//ImGui::EndPopup();
}

CE::MetaType CE::ContentBrowserEditorSystem::Reflect()
{
	MetaType type{ MetaType::T<ContentBrowserEditorSystem>{}, "ContentBrowserEditorSystem", MetaType::Base<EditorSystem>{} };
	type.GetProperties().Add(Props::sEditorSystemDefaultOpenTag);
	return type;
}
