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
	// The index inside the rootfolder when calling MakeFolderGraph
	constexpr uint32 sIndexOfEngineAssets = 0;
	constexpr uint32 sIndexOfGameAssets = 1;

	constexpr std::string_view sEngineAssetsDisplayName = "EngineAssets";
	constexpr std::string_view sGameAssetsDisplayName = "GameAssets";

	std::string sPopUpFolderRelativeToRoot{};
	std::string sPopUpNewAssetName{};
	bool sPopUpIsEngineAsset{};
	const CE::MetaType* sPopUpAssetClass{};
}

CE::ContentBrowserEditorSystem::ContentBrowserEditorSystem() :
	EditorSystem("ContentBrowser"),
	mFolderGraph(MakeFolderGraph())
{
}

void CE::ContentBrowserEditorSystem::Tick(const float)
{
	ImGui::ShowDemoWindow();

	if (!Begin())
	{
		End();
		return;
	}

	ThumbnailEditorSystem* thumbnailEditorSystem = Editor::Get().TryGetSystem<ThumbnailEditorSystem>();

	if (thumbnailEditorSystem == nullptr)
	{
		LOG(LogEditor, Error, "Could not display content, no thumbnail editor system!");
		return;
	}

	ImGui::Splitter(true, &mFolderHierarchyPanelWidthPercentage, &mContentPanelWidthPercentage);

	if (ImGui::BeginChild("FolderHierarchy", { mFolderHierarchyPanelWidthPercentage, -2.0f }, false, ImGuiWindowFlags_NoScrollbar))
	{
		if (ImGui::Button(ICON_FA_PLUS))
		{
			//ImGui::OpenPopup(sAssetCreatorImGuiId.data());
		}

		ImGui::SetItemTooltip("Create a new asset");
		ImGui::SameLine();
		Search::Begin();

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		DisplayFolder(mFolderGraph);

		Search::End();
	}
	ImGui::EndChild();
	ImGui::SameLine();

	if (ImGui::BeginChild("ContentPanel", { mContentPanelWidthPercentage, -2.0f }, false, ImGuiWindowFlags_NoScrollbar))
	{

		ImGui::BeginTable("TableTest",
			std::max(static_cast<int>(ImGui::GetContentRegionAvail().x / (ThumbnailEditorSystem::sGeneratedThumbnailResolution.x + 10.0f)), 1),
			ImGuiTableColumnFlags_WidthFixed);

		if (mSelectedFolder != nullptr)
		{
			for (ContentFolder& folder : mSelectedFolder->mChildren)
			{
				DisplayItemInFolder(folder, *thumbnailEditorSystem);
			}

			for (const WeakAssetHandle<>& content : mSelectedFolder->mContent)
			{
				DisplayItemInFolder(content, *thumbnailEditorSystem);
			}
		}

		ImGui::EndTable();
	}
	ImGui::EndChild();

	End();
}

CE::ContentBrowserEditorSystem::ContentFolder CE::ContentBrowserEditorSystem::MakeFolderGraph()
{
	// Create the root folder to hold the top-level directories and files
	ContentFolder rootFolder{};

	rootFolder.mPath = "All";

	rootFolder.mChildren.emplace_back(sEngineAssetsDisplayName);
	rootFolder.mChildren.emplace_back(sGameAssetsDisplayName);

	std::string engineAssets = FileIO::Get().GetPath(FileIO::Directory::EngineAssets, "");
	std::string gameAssets = FileIO::Get().GetPath(FileIO::Directory::GameAssets, "");

	// Don't include the /, as some file paths will have a backward slash.
	// Testing for equality later would then cause some issues
	if (!engineAssets.empty())
	{
		engineAssets.pop_back();
	}

	if (!gameAssets.empty())
	{
		gameAssets.pop_back();
	}

	// Iterate through the provided paths
	for (WeakAssetHandle<> asset : AssetManager::Get().GetAllAssets())
	{
		const std::string fullPath = asset.GetFileOfOrigin().value_or(
			std::filesystem::path{ "Generated assets" }.operator/=(asset.GetMetaData().GetName())).string();

		std::filesystem::path currentPath{};
		std::filesystem::path relativePath{};
		ContentFolder* currentFolder = &rootFolder;

		if (fullPath.substr(0, engineAssets.size()) == engineAssets)
		{
			currentFolder = &rootFolder.mChildren[sIndexOfEngineAssets];
			relativePath = fullPath.substr(engineAssets.size() + 1);
		}
		else if (fullPath.substr(0, gameAssets.size()) == gameAssets)
		{
			currentFolder = &rootFolder.mChildren[sIndexOfGameAssets];
			relativePath = fullPath.substr(gameAssets.size() + 1);
		}
		else
		{
			currentFolder = &rootFolder;
			relativePath = fullPath;
		}

		// Traverse the folder structure and create necessary folders
		for (const std::filesystem::path& subPath : relativePath)
		{
			currentPath /= subPath;

			// Check if the subPath is a directory
			if (subPath != relativePath.filename())
			{
				// Search for the subPath in the current folder's children
				auto it = std::find_if(currentFolder->mChildren.begin(), currentFolder->mChildren.end(),
					[&subPath](const ContentFolder& folder)
					{
						return folder.mPath.filename() == subPath;
					});

				// If not found, create a new folder
				if (it == currentFolder->mChildren.end())
				{
					currentFolder->mChildren.emplace_back(currentPath);
					it = currentFolder->mChildren.end() - 1;
				}

				// Update the current folder pointer
				currentFolder = &(*it);
			}
		}

		// Sort alphabetically
		const auto whereToInsert = std::lower_bound(currentFolder->mContent.begin(), currentFolder->mContent.end(), asset,
			[](const WeakAssetHandle<>& lhs, const WeakAssetHandle<>& rhs)
			{
				return lhs.GetMetaData().GetName() < rhs.GetMetaData().GetName();
			});
		currentFolder->mContent.emplace(whereToInsert, std::move(asset));
	}

	// Return the root folder, which contains the entire folder structure
	return rootFolder;
}

void CE::ContentBrowserEditorSystem::DisplayFolder(ContentFolder& folder)
{
	const bool displayAsTreeNode = !folder.mChildren.empty();

	Search::BeginCategory(folder.mPath.filename().string(),
		[&folder, this, displayAsTreeNode](std::string_view folderName) -> bool
		{
			bool isOpen{};
			if (displayAsTreeNode)
			{
				isOpen = ImGui::TreeNode(Format("##{}", folderName).data());
				ImGui::SameLine();
			}

			bool isSelected = mSelectedFolder == &folder;
			const ImVec2 selectableAreaSize = { ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() };

			if (ImGui::Selectable(Format("{} {}", isOpen ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, folderName).data(), &isSelected, 0, selectableAreaSize))
			{
				if (isSelected)
				{
					mSelectedFolder = &folder;
				}
				else
				{
					mSelectedFolder = nullptr;
				}
			}

			WeakAssetHandle receivedAsset = DragDrop::PeekAsset<Asset>();

			if (receivedAsset != nullptr
				&& receivedAsset.GetFileOfOrigin().has_value()
				&& receivedAsset.GetFileOfOrigin()->parent_path() != folder.mPath
				&& DragDrop::AcceptAsset())
			{
				AssetManager::Get().MoveAsset(receivedAsset, folder.mPath / receivedAsset.GetFileOfOrigin()->filename());
			}



			const std::string popUpName = Format("{}RightClickedDir", folder.mPath.string());

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

std::string CE::ContentBrowserEditorSystem::GetName(const ContentFolder& folder)
{
	return folder.mPath.filename().string();
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
		mSelectedFolder = &folder;
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
			mFolderGraph = MakeFolderGraph();
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
//		mFolderGraph = MakeFolderGraph();
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
