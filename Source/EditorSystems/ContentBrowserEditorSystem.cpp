#include "Precomp.h"
#include "EditorSystems/ContentBrowserEditorSystem.h"

#include "Assets/Asset.h"
#include "Core/AssetManager.h"
#include "Core/Editor.h"
#include "Utilities/Imgui/ImguiDragDrop.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Utilities/Search.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"

Engine::ContentBrowserEditorSystem::ContentBrowserEditorSystem() :
	EditorSystem("ContentBrowser")
{
}

void Engine::ContentBrowserEditorSystem::Tick(const float)
{
    if (!Begin())
    {
        End();
        return;
    }

    // Don't initialize it yet; we only need it the user presses Create new asset
    // or if they are not searching for a specific asset.
    std::vector<ContentFolder> folders{};

    ImGui::BeginDisabled(mAssetCreator.has_value());

    if (ImGui::Button("Create new asset"))
    {
        folders = MakeFolderGraph(AssetManager::Get().GetAllAssets());
        AssetCreator creator{};
        creator.mFileLocation = folders.empty() ? "" : folders[0].mPath.string();
        mAssetCreator = creator;
    }

    ImGui::EndDisabled();

    ImGui::SameLine();

    static std::string searchFor{};
	Search::DisplaySearchBar(searchFor);

    if (!searchFor.empty())
    {
		Search::Choices<Asset> assetsThatMatchSearch = Search::CollectChoices<Asset>();
        Search::EraseChoicesThatDoNotMatch(searchFor, assetsThatMatchSearch);

        for (Search::Choice<Asset>& choice : assetsThatMatchSearch)
        {
            DisplayAsset(std::move(choice.mValue));
        }

        End();
        return;
    }

    folders = MakeFolderGraph(AssetManager::Get().GetAllAssets());
    DisplayAssetCreator(folders);

    for (ContentFolder& folder : folders)
    {
        DisplayDirectory(std::move(folder));
    }

    End();
}

// Generated using chatgpt, but tbh it was so broken i am going to claim 50% of the credit
std::vector<Engine::ContentBrowserEditorSystem::ContentFolder> Engine::ContentBrowserEditorSystem::MakeFolderGraph(std::vector<WeakAsset<Asset>>&& assets)
{
    // Create the root folder to hold the top-level directories and files
    ContentFolder rootFolder{};

    // Iterate through the provided paths
    for (auto& asset : assets)
    {
        const std::filesystem::path& fullPath = asset.GetFileOfOrigin().value_or("Generated assets");
        std::filesystem::path currentPath{};
        const std::filesystem::path& relativePath = fullPath.relative_path();
        ContentFolder* currentFolder = &rootFolder;

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

        currentFolder->mContent.push_back(std::move(asset));
    }

    // Return the root folder, which contains the entire folder structure
    return rootFolder.mChildren;
}

void Engine::ContentBrowserEditorSystem::DisplayDirectory(ContentFolder&& folder)
{
    const bool isOpen = ImGui::TreeNode(folder.mPath.filename().string().c_str());

    std::optional<WeakAsset<Asset>> receivedAsset = DragDrop::PeekAsset<Asset>();
    if (receivedAsset.has_value()
        && receivedAsset->GetFileOfOrigin().has_value()
        && receivedAsset->GetFileOfOrigin()->parent_path() != folder.mPath
        && DragDrop::AcceptAsset())
    {
        AssetManager::Get().MoveAsset(*receivedAsset, folder.mPath / receivedAsset->GetFileOfOrigin()->filename());
    }

    if (isOpen)
    {
        for (ContentFolder& child : folder.mChildren)
        {
            DisplayDirectory(std::move(child));
        }

        ImGui::Indent();

        for (WeakAsset<Asset>& asset : folder.mContent)
        {
            DisplayAsset(std::move(asset));
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}

void Engine::ContentBrowserEditorSystem::DisplayAsset(WeakAsset<Asset>&& asset) const
{
    if (ImGui::Button(asset.GetName().c_str()))
    {
        OpenAsset(asset);
    }

    // Looks scary because we may end up deleting the asset,
    // but a user can't drag an asset at the same time as they
    // click the delete button
    DragDrop::SendAsset(asset.GetName());

    if (ImGui::BeginItemTooltip())
    {
        ImGui::Text("Name: %s", asset.GetName().c_str());
        ImGui::Text("Type: %s", asset.GetAssetClass().GetName().c_str());

        if (asset.GetFileOfOrigin().has_value())
        {
            ImGui::Text("File: %s", asset.GetFileOfOrigin()->string().c_str());
        }
        else
        {
            ImGui::Text("Generated at runtime");
        }

        if (const std::optional<std::filesystem::path> importedFromFile = asset.GetImportedFromFile();
            importedFromFile.has_value())
        {
            ImGui::Text("Imported from: %s", importedFromFile->string().c_str());
        }

        ImGui::Text("NumOfReferences: %d", static_cast<int>(asset.NumOfReferences()));
        ImGui::EndTooltip();
    }

    if (ImGui::BeginPopupContextItem(std::string{ asset.GetName() }.append("##ContextItem").c_str()))
    {
        if (const std::optional<std::filesystem::path> importedFromFile = asset.GetImportedFromFile();
            importedFromFile.has_value()
            && ImGui::Button("Reimport"))
        {
            AssetManager::Get().Import(importedFromFile.value());
        }

        if (ImGui::Button("Delete"))
        {
            AssetManager::Get().DeleteAsset(std::move(asset));
        }

        ImGui::EndPopup();
    }
}

void Engine::ContentBrowserEditorSystem::OpenAsset(WeakAsset<Asset> asset) const
{
    Editor::Get().TryOpenAssetForEdit(asset);
}

void Engine::ContentBrowserEditorSystem::DisplayAssetCreator(const std::vector<ContentFolder>& contentFolders)
{
    if (!mAssetCreator.has_value())
    {
        return;
    }

    bool isOpen = true;
    if (ImGui::Begin("Asset creator", &isOpen))
    {
        if (ImGui::BeginCombo("Type", mAssetCreator->mClass == nullptr ? "None" : mAssetCreator->mClass->GetName().c_str()))
        {
            std::function<void(const MetaType&)> displayChildren =
                [&](const MetaType& type)
                {
                    for (const MetaType& child : type.GetDirectDerivedClasses())
                    {
                        if (ImGui::Button(child.GetName().c_str()))
                        {
                            mAssetCreator->mClass = &child;
                        }
                        displayChildren(child);
                    }
                };
            const MetaType* const assetType = MetaManager::Get().TryGetType<Asset>();
            ASSERT(assetType != nullptr);
            displayChildren(*assetType);

            ImGui::EndCombo();
        }

        ShowInspectUI("Name", mAssetCreator->mAssetName);
        ShowInspectUI("File location", mAssetCreator->mFileLocation);

        ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.0f, 0.0f, 1.0f });

        const MetaType* const assetClass = mAssetCreator->mClass;

        bool canCreate = true;

        if (assetClass == nullptr)
        {
            ImGui::TextUnformatted("No type selected.");
            canCreate = false;
        }
        else if (assetClass->IsExactly(MakeTypeId<Asset>())
            || !assetClass->IsDerivedFrom<Asset>())
        {
            ImGui::Text("%s is not a valid type.", assetClass->GetName().c_str());
            canCreate = false;
        }

        if (AssetManager::Get().TryGetWeakAsset(Name{ mAssetCreator->mAssetName }).has_value())
        {
            ImGui::Text("There is already an asset with the name %s", mAssetCreator->mAssetName.c_str());
            canCreate = false;
        }

        std::filesystem::path outputRootFolder = mAssetCreator->mFileLocation;

        while (outputRootFolder.has_parent_path())
        {
            outputRootFolder = outputRootFolder.parent_path();
        }

        if (std::find_if(contentFolders.begin(), contentFolders.end(),
            [&outputRootFolder](const ContentFolder& folder)
            {
                return folder.mPath.filename() == outputRootFolder.string();
            }) == contentFolders.end())
        {
            ImGui::Text("Folder is not inside one of the existing root folders");
            canCreate = false;
        }

        const std::filesystem::path actualOutputFile = (std::filesystem::path{ mAssetCreator->mFileLocation }
        / mAssetCreator->mAssetName).replace_extension(AssetManager::sAssetExtension);

        if (exists(actualOutputFile))
        {
            ImGui::Text("There is already a file at %s", actualOutputFile.string().c_str());
            canCreate = false;
        }

        ImGui::PopStyleColor();

        ImGui::BeginDisabled(!canCreate);

        if (ImGui::Button(Format("Save to {}", actualOutputFile.string()).c_str()))
        {
	        const AssetSaveInfo saveInfo{ mAssetCreator->mAssetName, *assetClass };
            const bool success = saveInfo.SaveToFile(actualOutputFile);

            if (success)
            {
                std::optional<WeakAsset<Asset>> newAsset = AssetManager::Get().AddAsset(actualOutputFile);

                if (newAsset.has_value())
                {
                    Editor::Get().TryOpenAssetForEdit(*newAsset);
                }

                isOpen = false;
            }
            else
            {
                LOG(LogEditor, Error, "Failed to create new asset {}, the file {} could not be saved to",
                    mAssetCreator->mAssetName, actualOutputFile.string());
            }
        }

        ImGui::EndDisabled();
    }

    ImGui::End();

    if (!isOpen)
    {
        mAssetCreator.reset();
    }
}

Engine::MetaType Engine::ContentBrowserEditorSystem::Reflect()
{
    MetaType type{MetaType::T<ContentBrowserEditorSystem>{}, "ContentBrowserEditorSystem", MetaType::Base<EditorSystem>{} };
    type.GetProperties().Add(Props::sEditorSystemDefaultOpenTag);
    return type;
}
