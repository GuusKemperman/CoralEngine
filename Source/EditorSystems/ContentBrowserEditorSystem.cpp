#include "Precomp.h"
#include "EditorSystems/ContentBrowserEditorSystem.h"

#include <imgui/imgui_internal.h>

#include "Assets/Asset.h"
#include "Core/AssetManager.h"
#include "Core/Editor.h"
#include "Core/FileIO.h"
#include "Core/Input.h"
#include "EditorSystems/ImporterSystem.h"
#include "Utilities/Imgui/ImguiDragDrop.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Utilities/Search.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"
#include "Utilities/StringFunctions.h"

namespace
{
    // The index inside the rootfolder when calling MakeFolderGraph
    constexpr uint32 sIndexOfEngineAssets = 0;
    constexpr uint32 sIndexOfGameAssets = 1;

    constexpr std::string_view sEngineAssetsDisplayName = "EngineAssets";
    constexpr std::string_view sGameAssetsDisplayName = "GameAssets";

    constexpr std::string_view sAssetCreatorImGuiId = "AssetCreator";
    constexpr std::string_view sAssetRightClickPopUpImGuiId = "AssetContext";

    std::string sRightClickedAsset{};
    bool sWasAssetJustRightClicked{};

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
    if (!Begin())
    {
        End();
        return;
    }

    if (ImGui::Button(ICON_FA_PLUS))
    {
        ImGui::OpenPopup(sAssetCreatorImGuiId.data());
    }

    ImGui::SetItemTooltip("Create a new asset");
    
    Search::Begin("ContentBrowserSearch");

    sWasAssetJustRightClicked = false;

    for (const ContentFolder& folder : mFolderGraph)
    {
        DisplayDirectory(folder);
    }

    Search::End();

    if (sWasAssetJustRightClicked)
    {
        ImGui::OpenPopup(sAssetRightClickPopUpImGuiId.data());
    }

    DisplayAssetCreatorPopUp();
    DisplayAssetRightClickPopUp();

    End();
}

// Generated using chatgpt, but tbh it was so broken i am going to claim 50% of the credit
std::vector<CE::ContentBrowserEditorSystem::ContentFolder> CE::ContentBrowserEditorSystem::MakeFolderGraph()
{
    // Create the root folder to hold the top-level directories and files
    ContentFolder rootFolder{};

    rootFolder.mChildren.emplace_back(sEngineAssetsDisplayName);
    rootFolder.mChildren.emplace_back(sGameAssetsDisplayName);

    const std::string engineAssets = FileIO::Get().GetPath(FileIO::Directory::EngineAssets, "");
    const std::string gameAssets = FileIO::Get().GetPath(FileIO::Directory::GameAssets, "");

    // Iterate through the provided paths
    for (WeakAsset<> asset : AssetManager::Get().GetAllAssets())
    {
        const std::string fullPath = asset.GetFileOfOrigin().value_or("Generated assets").string();

        std::filesystem::path currentPath{};
        std::filesystem::path relativePath{};
        ContentFolder* currentFolder = &rootFolder;

        if (fullPath.substr(0, engineAssets.size()) == engineAssets)
        {
            currentFolder = &rootFolder.mChildren[sIndexOfEngineAssets];
            relativePath = fullPath.substr(engineAssets.size());
        }
        else if (fullPath.substr(0, gameAssets.size()) == gameAssets)
        {
            currentFolder = &rootFolder.mChildren[sIndexOfGameAssets];
            relativePath = fullPath.substr(gameAssets.size());
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
            [](const WeakAsset<>& sl, const WeakAsset<>& sr)
            {
                return sl.GetMetaData().GetName() < sr.GetMetaData().GetName();
            });
        currentFolder->mContent.insert(whereToInsert, std::move(asset));
    }

    // Return the root folder, which contains the entire folder structure
    return rootFolder.mChildren;
}

void CE::ContentBrowserEditorSystem::DisplayDirectory(const ContentFolder& folder)
{
    Search::BeginCategory(folder.mPath.filename().string(),
        [&folder](std::string_view folderName) -> bool
        {
            const bool isOpen = ImGui::TreeNode(folderName.data());

            std::optional<WeakAsset<Asset>> receivedAsset = DragDrop::PeekAsset<Asset>();
            if (receivedAsset.has_value()
                && receivedAsset->GetFileOfOrigin().has_value()
                && receivedAsset->GetFileOfOrigin()->parent_path() != folder.mPath
                && DragDrop::AcceptAsset())
            {
                AssetManager::Get().MoveAsset(*receivedAsset, folder.mPath / receivedAsset->GetFileOfOrigin()->filename());
            }

            const std::string popUpName = Format("{}RightClickedDir", folder.mPath.string());

            if (ImGui::IsItemClicked(1))
            {
                ImGui::OpenPopup(popUpName.c_str());
            }

            if (ImGui::BeginPopup(popUpName.c_str()))
            {
                if (ImGui::MenuItem("Reimport"))
                {
                    std::function<void(const ContentFolder&)> importRecursive = [&importRecursive](const ContentFolder& folder)
                        {
                            for (const WeakAsset<Asset>& asset : folder.mContent)
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

                ImGui::EndPopup();
            }

            return isOpen;
        });

    for (const ContentFolder& child : folder.mChildren)
    {
        DisplayDirectory(std::move(child));
    }

    for (const WeakAsset<Asset>& asset : folder.mContent)
    {
        DisplayAsset(std::move(asset));
    }

    Search::TreePop();
}

void CE::ContentBrowserEditorSystem::DisplayAsset(const WeakAsset<Asset>& asset) const
{
    if (Search::AddEntry(asset.GetMetaData().GetName(),
        [asset](std::string_view) -> bool
        {
            bool returnValue{};

            if (Editor::Get().IsThereAnEditorTypeForAssetType(asset.GetMetaData().GetClass().GetTypeId()))
            {
                if (ImGui::Button(asset.GetMetaData().GetName().c_str()))
                {
                    returnValue = true;
                }
            }
            else
            {
                ImGui::TextUnformatted(asset.GetMetaData().GetName().c_str());
            }

            if (ImGui::IsItemClicked(1))
            {
                sRightClickedAsset = asset.GetMetaData().GetName();
                sWasAssetJustRightClicked = true;
            }

            DragDrop::SendAsset(asset.GetMetaData().GetName());

            if (ImGui::BeginItemTooltip())
            {
                ImGui::Text("Name: %s", asset.GetMetaData().GetName().c_str());
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

                ImGui::Text("NumOfReferences: %d", static_cast<int>(asset.NumOfReferences()));
                ImGui::EndTooltip();
            }

            return returnValue;
        }))
    {
        OpenAsset(asset);
    }
}

void CE::ContentBrowserEditorSystem::OpenAsset(WeakAsset<Asset> asset) const
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

    if (AssetManager::Get().TryGetWeakAsset(Name{ assetName }).has_value())
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

void CE::ContentBrowserEditorSystem::DisplayAssetCreatorPopUp()
{
    if (!ImGui::BeginPopup(sAssetCreatorImGuiId.data()))
    {
        return;
    }

	if (ImGui::BeginCombo("Type", sPopUpAssetClass == nullptr ? "None" : sPopUpAssetClass->GetName().c_str()))
	{
	    std::function<void(const MetaType&)> displayChildren =
	        [&](const MetaType& type)
	        {
	            for (const MetaType& child : type.GetDirectDerivedClasses())
	            {
	                if (ImGui::Button(child.GetName().c_str()))
	                {
	                    sPopUpAssetClass = &child;
	                }
	                displayChildren(child);
	            }
	        };
	    const MetaType* const assetType = MetaManager::Get().TryGetType<Asset>();
	    ASSERT(assetType != nullptr);
	    displayChildren(*assetType);

	    ImGui::EndCombo();
	}

	bool anyErrors = false;
    anyErrors |= DisplayNameUI(sPopUpNewAssetName);

    const FilePathUIResult fileUIResult = DisplayFilepathUI(sPopUpFolderRelativeToRoot, sPopUpIsEngineAsset, sPopUpNewAssetName);

    anyErrors |= fileUIResult.mAnyErrors;

    PushError();

	if (sPopUpAssetClass == nullptr)
	{
	    ImGui::TextUnformatted("No type selected.");
        anyErrors = true;
	}
	else if (sPopUpAssetClass->IsExactly(MakeTypeId<Asset>())
	    || !sPopUpAssetClass->IsDerivedFrom<Asset>())
	{
	    ImGui::Text("%s is not a valid type.", sPopUpAssetClass->GetName().c_str());
	    anyErrors = true;
	}

    PopError();

	ImGui::BeginDisabled(anyErrors);

	if (ImGui::Button(Format("Save to {}", fileUIResult.mPathToShowUser.string()).c_str()))
	{
        std::optional<WeakAsset<Asset>> newAsset = AssetManager::Get().NewAsset(*sPopUpAssetClass, fileUIResult.mActualFullPath);

        if (newAsset.has_value())
        {
            Editor::Get().TryOpenAssetForEdit(*newAsset);

            // Adding assets is not considered to be a volatile
			// action, our system will not restart, and we
			// have to explicitly state we want to recreate our
			// folder graph.
            mFolderGraph = MakeFolderGraph();
        }
	}

	ImGui::EndDisabled();


	ImGui::EndPopup();
}

void CE::ContentBrowserEditorSystem::DisplayAssetRightClickPopUp()
{
    if (!ImGui::BeginPopup(sAssetRightClickPopUpImGuiId.data()))
    {
        return;
    }

    std::optional<WeakAsset<Asset>> asset = AssetManager::Get().TryGetWeakAsset(sRightClickedAsset);

    if (!asset.has_value())
    {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return;
    }

    const Input& input = Input::Get();

    if (ImGui::BeginMenu("Rename##Menu"))
    {
        PushError();

        ImGui::TextUnformatted("This will break all existing references to the asset!");

        PopError();

        const bool anyErrors = DisplayNameUI(sPopUpNewAssetName);

        ImGui::BeginDisabled(anyErrors);

        if (ImGui::Button("Rename"))
        {
            AssetManager::Get().RenameAsset(*asset, sPopUpNewAssetName);
        }

        ImGui::EndDisabled();

        ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Duplicate"))
    {
        const std::string copyName = StringFunctions::CreateUniqueName(asset->GetMetaData().GetName(),
            [](std::string_view name)
            {
                return !AssetManager::Get().TryGetWeakAsset(name).has_value();
            });

        std::filesystem::path copyPath = asset->GetFileOfOrigin().value_or(FileIO::Get().GetPath(FileIO::Directory::GameAssets, copyName)).parent_path() / copyName;
        copyPath.replace_extension(AssetManager::sAssetExtension);

        std::optional<WeakAsset<Asset>> newAsset = AssetManager::Get().Duplicate(*asset, copyPath);

        if (newAsset.has_value())
        {
            Editor::Get().TryOpenAssetForEdit(*newAsset);

            // Adding assets is not considered to be a volatile
            // action, our system will not restart, and we
            // have to explicitly state we want to recreate our
            // folder graph.
            mFolderGraph = MakeFolderGraph();
        }
    }

    if (const std::optional<AssetFileMetaData::ImporterInfo>& importerInfo = asset->GetMetaData().GetImporterInfo();
        importerInfo.has_value()
        && ImGui::MenuItem("Reimport"))
    {
        Reimport(*asset);
    }

    if (ImGui::MenuItem("Delete", "Del")
        || input.WasKeyboardKeyPressed(Input::KeyboardKey::Delete))
    {
        AssetManager::Get().DeleteAsset(std::move(*asset));
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void CE::ContentBrowserEditorSystem::Reimport(const WeakAsset<Asset>& asset)
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

CE::MetaType CE::ContentBrowserEditorSystem::Reflect()
{
    MetaType type{MetaType::T<ContentBrowserEditorSystem>{}, "ContentBrowserEditorSystem", MetaType::Base<EditorSystem>{} };
    type.GetProperties().Add(Props::sEditorSystemDefaultOpenTag);
    return type;
}
