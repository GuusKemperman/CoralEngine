#include "Precomp.h"
#include "EditorSystems/ImporterSystem.h"

#include "Core/Editor.h"
#include "Core/FileIO.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaTools.h"
#include "Utilities/ClassVersion.h"
#include "Utilities/Imgui/ImguiHelpers.h"

CE::ImporterSystem::ImporterSystem() :
	EditorSystem("ImporterSystem"),
	mWasImportingCancelled(std::make_shared<bool>(false)),
	mDirectoriesToWatch({
		DirToWatch{ FileIO::Get().GetPath(FileIO::Directory::EngineAssets, "") },
		DirToWatch{ FileIO::Get().GetPath(FileIO::Directory::GameAssets, "") }
		})
{
	const MetaType& importerType = MetaManager::Get().GetType<Importer>();

	const std::function addImporter =
		[&](const MetaType& type)
		{
			for (const MetaType& derived : type.GetDirectDerivedClasses())
			{
				FuncResult constructResult = derived.Construct();

				if (constructResult.HasError())
				{
					LOG(LogAssets, Error, "Importer {} has no default constructor and cannot be used. Did you override all the pure functions?",
						derived.GetName());
					continue;
				}

				auto importer = MakeUnique<Importer>(std::move(constructResult.GetReturnValue()));

				std::vector<std::filesystem::path> canImportExtensions = importer->CanImportExtensions();

				if (canImportExtensions.empty())
				{
					LOG(LogAssets, Warning, "Importer {} cannot import any extensions, return value of CanImportExtensions was empty",
						derived.GetName());
					continue;
				}

				for (const std::filesystem::path& extension : canImportExtensions)
				{
					if (extension.empty())
					{
						LOG(LogAssets, Warning, "Importer {} has invalid extension: Extension was empty",
							derived.GetName());
						goto nextImporter;
					}

					if (extension.string()[0] != '.')
					{
						LOG(LogAssets, Warning, "Importer {} has invalid extension {}: extensions must start with a period",
							derived.GetName(),
							extension.string());
						goto nextImporter;
					}

					if (extension == AssetManager::sAssetExtension)
					{
						LOG(LogAssets, Warning, "Importer {} has invalid extension {}: extensions cannot be the same as the asset extension \"{}\"",
							derived.GetName(),
							extension.string(),
							AssetManager::sAssetExtension);
						goto nextImporter;
					}

					if (const auto [existingImporterTypeId, existingImporter] = TryGetImporterForExtension(extension); existingImporter != nullptr)
					{
						[[maybe_unused]] const MetaType* existingImporterType = MetaManager::Get().TryGetType(existingImporterTypeId);

						LOG(LogAssets, Warning, "Importer {} has invalid extension {}: importer {} is already responsible for this extension",
							derived.GetName(),
							extension.string(),
							existingImporterType == nullptr ? "Unnamed importer" : existingImporterType->GetName());

						goto nextImporter;
					}
				}

				mImporters.emplace_back(derived.GetTypeId(), std::move(importer));

			nextImporter: {}
			}
		};
	addImporter(importerType);
}

CE::ImporterSystem::~ImporterSystem() = default;

void CE::ImporterSystem::Tick(const float)
{
	ImportAllOutOfDateFiles();
	RetrieveImportResultsFromFutures();

	ImGui::SetNextWindowSize({ 800.0f, 600.0f }, ImGuiCond_FirstUseEver);

	const bool hasContent = !mImportPreview.empty()
		|| !mFailedFiles.empty()
		|| !mImportFutures.empty();

	if (!mHasWindowPoppedUp 
		&& hasContent)
	{
		sIsOpen = true;
	}

	if (!sIsOpen
		|| !hasContent)
	{
		return;
	}

	if (!ImGui::Begin(GetName().c_str(), &sIsOpen))
	{
		ImGui::End();
		return;
	}

	mHasWindowPoppedUp = true;

	Preview();
	uint32 numOfConflicts = ShowDuplicateImportedAssetsErrors();
	numOfConflicts += ShowDuplicateAssetsErrors();
	numOfConflicts += ShowErrorsToWarnAboutDiscardChanges();
	numOfConflicts += ShowReadOnlyErrors();

	if (numOfConflicts != 0)
	{
		ImGui::TextWrapped(Format("{} conflicts found", numOfConflicts).c_str());
	}

	ImGui::BeginDisabled(numOfConflicts != 0 || !mImportFutures.empty());

	if(ImGui::Button("Import"))
	{
		Editor& editor = Editor::Get();

		std::shared_ptr<std::vector<ImportPreview>> preview = std::make_shared<std::vector<ImportPreview>>(std::move(mImportPreview));

		editor.Refresh(
			{
				Editor::RefreshRequest::Volatile,
				[preview]
				{
					FinishImporting(std::move(*preview));
				}
			});
	}

	ImGui::EndDisabled();

	if (!mImportFutures.empty())
	{
		ImGui::SetItemTooltip("Wait for the importing to finish");
	}

	ImGui::End();
}

void CE::ImporterSystem::Import(const std::filesystem::path& fileToImport, std::string_view reasonForImporting)
{
	// Check to see if we are already importing this file
	if (std::find_if(mImportFutures.begin(), mImportFutures.end(),
		[&fileToImport](const ImportFuture& future)
		{
			return future.mImportRequest.mFile == fileToImport;
		}) != mImportFutures.end())
	{
		return;
	}

	const auto [importerTypeId, importer] = TryGetImporterForExtension(fileToImport.extension());

	Logger::Get().Log(Format("Importing {}", fileToImport.string()), "LogEditor", LogSeverity::Message, __FILE__, __LINE__,
		[]
		{
			sIsOpen = true;
		});

	LOG(LogAssets, Message, "Importing {}", fileToImport.string());

	if (importer == nullptr)
	{
		LOG(LogAssets, Error, "No importer that can import {}.", fileToImport.string());
	}

	mImportFutures.emplace_back(
		ImportFuture
		{
			fileToImport,
			std::string{ reasonForImporting },
			std::async(std::launch::async, [fileToImport, importer, isCancelled = mWasImportingCancelled]() -> Importer::ImportResult
			{
				if (*isCancelled)
				{
					return std::nullopt;
				}

				return importer->Import(fileToImport);
			})
		});
}

void CE::ImporterSystem::ImportAllOutOfDateFiles()
{
	std::vector<ImportRequest> all = GetAllFilesToImport(mDirectoriesToWatch[0]);
	std::vector<ImportRequest> game = GetAllFilesToImport(mDirectoriesToWatch[1]);

	all.insert(all.end(), std::make_move_iterator(game.begin()), std::make_move_iterator(game.end()));

	for (const ImportRequest& assetToImport : all)
	{
		Import(assetToImport.mFile, assetToImport.mReasonForImporting);
	}
}

std::vector<CE::ImporterSystem::ImportRequest> CE::ImporterSystem::GetAllFilesToImport(DirToWatch& directory)
{
	std::vector<ImportRequest> importableAssets{};

	const std::filesystem::file_time_type dirWriteTime = std::filesystem::last_write_time(directory.mDirectory);
	if (directory.mDirWriteTimeWhenLastChecked >= dirWriteTime)
	{
		return importableAssets;
	}

	directory.mDirWriteTimeWhenLastChecked = dirWriteTime;

	for (const std::filesystem::directory_entry& dirEntry : std::filesystem::recursive_directory_iterator(directory.mDirectory))
	{
		if (!dirEntry.is_regular_file())
		{
			continue;
		}

		const std::filesystem::path& fileToImport = dirEntry.path();

		const auto [importerTypeId, importer] = TryGetImporterForExtension(fileToImport.extension());

		if (importer == nullptr)
		{
			continue;
		}

		bool wasPreviouslyImported = false;

		for (const WeakAsset<> asset : AssetManager::Get().GetAllAssets())
		{
			if (!WasImportedFrom(asset, fileToImport))
			{
				continue;
			}

			wasPreviouslyImported = true;

			const uint32 assetImporterVersion = asset.GetMetaData().GetImporterInfo()->mImporterVersion;

			const MetaType* const importerType = MetaManager::Get().TryGetType(importerTypeId);
			ASSERT(importerType != nullptr);

			if (assetImporterVersion != GetClassVersion(*importerType))
			{
				importableAssets.push_back(
					{
						fileToImport,
						Format("Asset {} was imported with an older version of the importer ({}). Reimporting...",
						fileToImport.string(),
						assetImporterVersion)
					}
				);
				break;
			}

			const uint32 currentAssetVersion = GetClassVersion(asset.GetMetaData().GetClass());

			if (asset.GetMetaData().GetAssetVersion() != currentAssetVersion)
			{
				importableAssets.push_back(
					{
						fileToImport,
						Format("Asset {} is out-of-date, version is {} (current is {}). The asset will be re-imported from {}",
					asset.GetMetaData().GetName(),
					asset.GetMetaData().GetAssetVersion(),
					currentAssetVersion,
					fileToImport.string())
					}
				);
				break;
			}

			if (asset.GetMetaData().GetMetaDataVersion() != AssetFileMetaData::GetCurrentMetaDataVersion())
			{
				importableAssets.push_back(
					{
						fileToImport,
						Format("Asset {} is out-of-date, metadata version is {} (current is {}). The asset will be re-imported from {}",
					asset.GetMetaData().GetName(),
					asset.GetMetaData().GetMetaDataVersion(),
					AssetFileMetaData::GetCurrentMetaDataVersion(),
					fileToImport.string())
					}
				);
				break;
			}
		}

		if (!wasPreviouslyImported)
		{
			importableAssets.push_back(
				{
					fileToImport,
					Format("New content detected at {}. Importing...",
					fileToImport.string())
				}
			);
		}
	}

	return importableAssets;
}

bool CE::ImporterSystem::WasImportedFrom(const WeakAsset<>& asset, const std::filesystem::path& file)
{
	return asset.GetMetaData().GetImporterInfo().has_value()
		// We only look at the filename, as the full path may be different.
		// For example the EngineAssets folder may be moved relative to the
		// working environment, or may always be different for several projects.
		//
		// So the relative path can change, but so can the absolute path;
		// one person might save their engine on the D: drive, while someone
		// else might save it in C:/Projects/Repos.
		//
		// Since neither option is ideal, we only look at the filename.
		&& asset.GetMetaData().GetImporterInfo()->mImportedFile.filename() == file.filename();
}

std::pair<CE::TypeId, std::shared_ptr<const CE::Importer>> CE::ImporterSystem::TryGetImporterForExtension(const std::filesystem::path& extension)
{
	for (const auto& [typeId, importer] : mImporters)
	{
		std::vector<std::filesystem::path> canImport = importer->CanImportExtensions();
		auto it = std::find(canImport.begin(), canImport.end(), extension);

		if (it != canImport.end())
		{
			return { typeId, importer };
		}
	}

	return { 0, nullptr };
}

bool CE::ImporterSystem::WouldAssetBeDeletedOrReplacedOnImporting(const WeakAsset<>& asset) const
{
	return WouldAssetBeDeletedOrReplacedOnImporting(asset, mImportPreview);
}

std::filesystem::path CE::ImporterSystem::GetPathToSaveAssetTo(const ImportPreview& preview) const
{
	return GetPathToSaveAssetTo(preview, mImportPreview);
}

std::filesystem::path CE::ImporterSystem::GetPathToSaveAssetTo(const ImportPreview& preview, const std::vector<ImportPreview>& toImport)
{
	std::string filename = preview.mImportedAsset.GetMetaData().GetName();

	for (char& ch : filename)
	{
		if (ch == '<'
			|| ch == '>'
			|| ch == ':'
			|| ch == '\"'
			|| ch == '/'
			|| ch == '\\'
			|| ch == '|'
			|| ch == '?'
			|| ch == '*'
			|| ch == ' '
			|| ch == '.'
			|| ch == ','
			)
		{
			ch = '_';
		}
	}

	const size_t numFromThisFile = std::count_if(toImport.begin(), toImport.end(),
		[&preview](const ImportPreview& other)
		{
			return preview.mImportRequest.mFile == other.mImportRequest.mFile;
		});

	std::filesystem::path dir = preview.mImportRequest.mFile.parent_path();

	if (numFromThisFile > 1)
	{
		dir /= preview.mImportRequest.mFile.filename().replace_extension();
	}

	return dir / filename.append(AssetManager::sAssetExtension);
}

bool CE::ImporterSystem::WouldAssetBeDeletedOrReplacedOnImporting(const WeakAsset<>& asset, const std::vector<ImportPreview>& toImport)
{
	if (!asset.GetMetaData().GetImporterInfo().has_value())
	{
		return false;
	}

	for (auto first = toImport.begin(), last = first; first != toImport.end(); first = last)
	{
		last = std::find_if(first, toImport.end(),
			[&first](const ImportPreview& preview)
			{
				return preview.mImportRequest.mFile != first->mImportRequest.mFile;
			});

		if (first->mImportRequest.mFile == asset.GetMetaData().GetImporterInfo()->mImportedFile)
		{
			return true;
		}
	}

	return false;
}

void CE::ImporterSystem::RetrieveImportResultsFromFutures()
{
	for (auto it = mImportFutures.begin(); it != mImportFutures.end();)
	{
		if (it->mImportResult.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready)
		{
			++it;
			continue;
		}

		Importer::ImportResult result = it->mImportResult.get();

		if (!result.has_value())
		{
			mFailedFiles.emplace_back(it->mImportRequest);
		}
		else
		{
			for (ImportedAsset& imported : *result)
			{
				mImportPreview.emplace_back(ImportPreview{ it->mImportRequest, std::move(imported) });
			}
		}

		it = mImportFutures.erase(it);
	}
}

void CE::ImporterSystem::Preview()
{
	if (!mImportFutures.empty())
	{
		const std::string::size_type numOfDots = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 3 + 1;
		ImGui::TextWrapped(Format("{} files remaining{}", mImportFutures.size(), std::string(numOfDots, '.')).c_str());
	}

	ImVec2 childSize = ImGui::GetContentRegionAvail();
	childSize.y *= .5f;

	if (!ImGui::BeginChild("Preview", childSize))
	{
		ImGui::EndChild();
		return;
	}

	{
		bool isOpenAlready{}, shouldDisplay{};
		for (const ImportRequest& file : mFailedFiles)
		{
			if (OpenErrorTab(isOpenAlready, shouldDisplay, "Some files failed to import - see Log"))
			{
				ImGui::TextWrapped(file.mFile.string().c_str());
			}
		}
		CloseErrorTab(shouldDisplay);
	}

	for (auto first = mImportPreview.begin(), last = first; first != mImportPreview.end(); first = last)
	{
		last = std::find_if(first, mImportPreview.end(),
			[&first](const ImportPreview& preview)
			{
				return preview.mImportRequest.mFile != first->mImportRequest.mFile;
			});

		bool removeButtonPressed{};

		const bool isHeaderOpen = ImGui::CollapsingHeaderWithButton(first->mImportRequest.mFile.string().c_str(), "X", &removeButtonPressed);

		if (removeButtonPressed)
		{
			last = mImportPreview.erase(first, last);
			continue;
		}

		if (!isHeaderOpen)
		{
			continue;
		}

		for (auto it = first; it != last; ++it)
		{
			ImGui::TextWrapped(it->mImportedAsset.GetMetaData().GetName().c_str());
		}
	}

	ImGui::EndChild();
}

uint32 CE::ImporterSystem::ShowDuplicateImportedAssetsErrors()
{
	uint32 numOfErrors{};
	std::vector<std::string> duplicateNames{};
	bool isOpen{}, shouldDisplay{};

	for (auto preview1 = mImportPreview.begin(); preview1 != mImportPreview.end(); ++preview1)
	{
		const auto& name = preview1->mImportedAsset.GetMetaData().GetName();

		if (std::find(duplicateNames.begin(), duplicateNames.end(), name) != duplicateNames.end())
		{
			continue;
		}

		for (auto preview2 = preview1 + 1; preview2 != mImportPreview.end(); ++preview2)
		{
			if (preview2->mImportedAsset.GetMetaData().GetName() != name)
			{
				continue;
			}

			// Oh noo, we are trying to import multiple assets with the same name
			if (OpenErrorTab(isOpen, shouldDisplay, "Duplicate names from currently imported"))
			{
				ImGui::TextWrapped(Format("The asset {} was imported from both {} amd {}. Choose one of them to exclude from the importing process.",
					name,
					preview1->mImportRequest.mFile.string(),
					preview2->mImportRequest.mFile.string()).c_str());
			}

			numOfErrors++;
		}
	}

	CloseErrorTab(shouldDisplay);
	return numOfErrors;
}

uint32 CE::ImporterSystem::ShowDuplicateAssetsErrors()
{
	bool isOpenAlready{}, shouldDisplay{};
	uint32 numOfErrors{};

	for (const ImportPreview& preview : mImportPreview)
	{
		const std::string& name = preview.mImportedAsset.GetMetaData().GetName();
		const std::optional<WeakAsset<Asset>> existingAsset = AssetManager::Get().TryGetWeakAsset(name);

		if (!existingAsset.has_value()
			|| WasImportedFrom(*existingAsset, preview.mImportRequest.mFile))
		{
			continue;
		}

		if (OpenErrorTab(isOpenAlready, shouldDisplay, "Name was taken"))
		{
			ImGui::TextWrapped(Format("Could not import {} from {}, there is already an asset with this name (see {})",
				name,
				preview.mImportRequest.mFile.string(),
				existingAsset->GetFileOfOrigin().value_or("assets generated at runtime").string()).c_str());
		}
		++numOfErrors;
	}

	CloseErrorTab(shouldDisplay);
	return numOfErrors;
}

uint32 CE::ImporterSystem::ShowErrorsToWarnAboutDiscardChanges()
{
	bool isOpenAlready{}, shouldDisplay{};
	uint32 numOfErrors{};

	for (const ImportPreview& preview : mImportPreview)
	{
		const std::string& name = preview.mImportedAsset.GetMetaData().GetName();
		const std::optional<WeakAsset<Asset>> existingAsset = AssetManager::Get().TryGetWeakAsset(name);

		if (!existingAsset.has_value()
			|| !WasImportedFrom(*existingAsset, preview.mImportRequest.mFile)
			|| !existingAsset->GetMetaData().GetImporterInfo()->mWereEditsMadeAfterImporting
			|| !WouldAssetBeDeletedOrReplacedOnImporting(*existingAsset))
		{
			continue;
		}

		if (OpenErrorTab(isOpenAlready, shouldDisplay, "Changes would be lost"))
		{
			ImGui::TextWrapped(Format("{} from {} has been modified after it was imported. Reimporting it would lead to these changes being lost. Delete the asset to confirm you want these changes to be lost.",
				name,
				preview.mImportRequest.mFile.string()).c_str());
		}
		++numOfErrors;
	}

	CloseErrorTab(shouldDisplay);
	return numOfErrors;
}

uint32 CE::ImporterSystem::ShowReadOnlyErrors()
{
	bool isOpenAlready{}, shouldDisplay{};
	uint32 numOfErrors{};

	for (WeakAsset<> asset : AssetManager::Get().GetAllAssets())
	{
		if (WouldAssetBeDeletedOrReplacedOnImporting(asset)
			&& std::filesystem::exists(*asset.GetFileOfOrigin())
			&& (std::filesystem::status(*asset.GetFileOfOrigin()).permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none)
		{
			if (OpenErrorTab(isOpenAlready, shouldDisplay, "Files are read-only"))
			{
				ImGui::TextWrapped(Format("{} is read-only",
					asset.GetFileOfOrigin()->string()).c_str());
			}

			++numOfErrors;
		}
	}

	CloseErrorTab(shouldDisplay);
	return numOfErrors;
}

void CE::ImporterSystem::FinishImporting(std::vector<ImportPreview> toImport)
{
	std::vector<WeakAsset<>> assetsToEraseEntirely{};

	AssetManager& assetManager = AssetManager::Get();

	for (WeakAsset<> asset : assetManager.GetAllAssets())
	{
		if (!WouldAssetBeDeletedOrReplacedOnImporting(asset, toImport))
		{
			continue;
		}

		const std::filesystem::path& existingImportedAssetFile = *asset.GetFileOfOrigin();

		auto newImport = std::find_if(toImport.begin(), toImport.end(),
			[asset](const ImportPreview& toImport)
			{
				return toImport.mImportedAsset.GetMetaData().GetName() == asset.GetMetaData().GetName();
			});

		if (newImport == toImport.end())
		{
			// During out previous importing, we created this asset. But now that we are importing again,
			// this asset was not produced.
			assetsToEraseEntirely.push_back(asset);
		}
		else
		{
			const std::filesystem::path newAssetFile = GetPathToSaveAssetTo(*newImport, toImport);

			if (newAssetFile != existingImportedAssetFile)
			{
				// Update the internal mFileOfOrigin
				assetManager.MoveAsset(asset, newAssetFile);
				ASSERT(existingImportedAssetFile == newAssetFile);
			}
		}

		// Delete existing files that were generated the last time we imported this asset
		if (std::filesystem::exists(existingImportedAssetFile))
		{
			LOG(LogAssets, Message, "Deleting file {} created during previous importation", existingImportedAssetFile.string());

			std::error_code err{};
			if (!std::filesystem::remove(existingImportedAssetFile, err))
			{
				LOG(LogAssets, Error, "Could not delete file {} - {}", existingImportedAssetFile.string(), err.message());
			}
		}
	}

	for (WeakAsset<Asset>& assetToErase : assetsToEraseEntirely)
	{
		assetManager.DeleteAsset(std::move(assetToErase));
	}
	assetsToEraseEntirely.clear();

	// Finally, we can safely import
	for (const ImportPreview& imported : toImport)
	{
		const std::filesystem::path file = GetPathToSaveAssetTo(imported, toImport);

		if (!imported.mImportedAsset.SaveToFile(file))
		{
			LOG(LogAssets, Error, "Importing partially failed: Could not save {} to {}", 
				imported.mImportedAsset.GetMetaData().GetName(), 
				file.string());
			continue;
		}

		if (assetManager.TryGetWeakAsset(imported.mImportedAsset.GetMetaData().GetName()).has_value())
		{
			continue;
		}

		if (assetManager.OpenAsset(file).has_value())
		{
			LOG(LogAssets, Verbose, "Imported {} from {} to {}",
				imported.mImportedAsset.GetMetaData().GetName(),
				imported.mImportRequest.mFile.string(),
				file.string());
		}
		else
		{
			LOG(LogAssets, Warning, "Failed to open {} from {}, engine might require restart in order for this asset to show up.",
				imported.mImportedAsset.GetMetaData().GetName(),
				file.string());
		}
	}
};

bool CE::ImporterSystem::OpenErrorTab(bool& isOpenAlready, bool& shouldDisplay, std::string_view name)
{
	if (isOpenAlready)
	{
		return shouldDisplay;
	}

	shouldDisplay = ImGui::TreeNode(name.data());
	isOpenAlready = true;
	return shouldDisplay;
}

void CE::ImporterSystem::CloseErrorTab(bool shouldClose)
{
	if (shouldClose)
	{
		ImGui::TreePop();
	}
}

CE::MetaType CE::ImporterSystem::Reflect()
{
	MetaType type{ MetaType::T<ImporterSystem>{}, "ImporterSystem", MetaType::Base<EditorSystem>{} };
	type.GetProperties().Add(Props::sEditorSystemAlwaysOpenTag);
	return type;
}
