#include "Precomp.h"
#include "EditorSystems/ImporterSystem.h"

#include "Core/FileIO.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaTools.h"
#include "Utilities/ClassVersion.h"

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

	ImGui::SetNextWindowSize({ 800.0f, 600.0f }, ImGuiCond_FirstUseEver);

	if (!sIsOpen)
	{
		// Open the window if all our futures are ready
		sIsOpen = !std::any_of(mImportFutures.begin(), mImportFutures.end(),
			[](const ImportFuture& future)
			{
				return future.mImportResult.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready;
			});

		if (!sIsOpen)
		{
			return;
		}
	}

	if (!ImGui::Begin(GetName().c_str(), &sIsOpen))
	{
		ImGui::End();
		return;
	}

	const std::string::size_type numOfDots = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() % 3 + 1;
	const std::string loadedText = "Loading" + std::string( numOfDots, '.' );

	for (ImportFuture& future : mImportFutures)
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
		if (ImGui::TreeNode(future.mFile.string().c_str()))
		{
			if (future.mImportResult.wait_for(std::chrono::seconds{ 0 }) != std::future_status::ready)
			{
				ImGui::TextUnformatted(loadedText.c_str());
			}
			else
			{
				const auto& result = future.mImportResult.get();

				if (!result.has_value())
				{
					ImGui::TextUnformatted("Importing failed, check the output log for more details.");
				}
				else
				{
					for (const AssetLoadInfo& imported : *result)
					{
						ImGui::TextUnformatted(imported.GetName().c_str());
					}
				}
			}

			ImGui::TreePop();
		}
	}

	ImGui::End();
}

void CE::ImporterSystem::Import(const std::filesystem::path& fileToImport, std::string_view reasonForImporting)
{
	// Check to see if we are already importing this file
	if (std::find_if(mImportFutures.begin(), mImportFutures.end(),
		[&fileToImport](const ImportFuture& future)
		{
			return future.mFile == fileToImport;
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
			std::async(std::launch::async, [fileToImport, importer, isCancelled = mWasImportingCancelled]() -> std::optional<std::vector<AssetLoadInfo>>
			{
				if (*isCancelled)
				{
					return std::nullopt;
				}

				std::optional<std::vector<ImportedAsset>> importResult = importer->Import(fileToImport);

				if (!importResult.has_value())
				{
					return std::nullopt;
				}

				return std::vector<AssetLoadInfo>{ importResult->data(), importResult->data() + importResult->size() };
			})
		});
}

void CE::ImporterSystem::ImportAllOutOfDateFiles()
{
	std::vector<FileToImport> all = GetAllFilesToImport(mDirectoriesToWatch[0]);
	std::vector<FileToImport> game = GetAllFilesToImport(mDirectoriesToWatch[1]);

	all.insert(all.end(), std::make_move_iterator(game.begin()), std::make_move_iterator(game.end()));

	for (const FileToImport& assetToImport : all)
	{
		Import(assetToImport.mFile, assetToImport.mReasonForImporting);
	}
}

std::vector<CE::ImporterSystem::FileToImport> CE::ImporterSystem::GetAllFilesToImport(DirToWatch& directory)
{
	std::vector<FileToImport> importableAssets{};

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

		const std::filesystem::file_time_type importableAssetLastWriteTime = std::filesystem::last_write_time(fileToImport);

		bool wasPreviouslyImported = false;

		for (const WeakAsset<> asset : AssetManager::Get().GetAllAssets())
		{
			if (!WasImportedFrom(asset, fileToImport))
			{
				continue;
			}

			wasPreviouslyImported = true;

			const uint32 assetImporterVersion = asset.GetImporterInfo()->mImporterVersion;

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

			const uint32 currentAssetVersion = GetClassVersion(asset.GetAssetClass());

			if (asset.GetAssetVersion() != currentAssetVersion)
			{
				importableAssets.push_back(
					{
						fileToImport,
						Format("Asset {} is out-of-date, version is {} (current is {}). The asset will be re-imported from {}",
					asset.GetName(),
					asset.GetAssetVersion(),
					currentAssetVersion,
					fileToImport.string())
					}
				);
				break;
			}

			if (asset.GetMetaDataVersion() != AssetFileMetaData::GetCurrentMetaDataVersion())
			{
				importableAssets.push_back(
					{
						fileToImport,
						Format("Asset {} is out-of-date, metadata version is {} (current is {}). The asset will be re-imported from {}",
					asset.GetName(),
					asset.GetMetaDataVersion(),
					AssetFileMetaData::GetCurrentMetaDataVersion(),
					fileToImport.string())
					}
				);
				break;
			}

			if (asset.GetImporterInfo()->mImportedFromFileWriteTimeAtTimeOfImporting < importableAssetLastWriteTime)
			{
				importableAssets.push_back(
					{
						fileToImport,
						Format("Changes to {} detected. Reimporting...",
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
	return asset.GetImporterInfo().has_value()
		// We only look at the filename, as the full path may be different.
		// For example the EngineAssets folder may be moved relative to the
		// working environment, or may always be different for several projects.
		//
		// So the relative path can change, but so can the absolute path;
		// one person might save their engine on the D: drive, while someone
		// else might save it in C:/Projects/Repos.
		//
		// Since neither option is ideal, we only look at the filename.
		&& asset.GetImportedFromFile()->filename() == file.filename();
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

CE::MetaType CE::ImporterSystem::Reflect()
{
	MetaType type{ MetaType::T<ImporterSystem>{}, "ImporterSystem", MetaType::Base<EditorSystem>{} };
	type.GetProperties().Add(Props::sEditorSystemAlwaysOpenTag);
	return type;
}

/*	auto importLambda = [this, path]
		{


			if (!importedAssets.has_value())
			{
				LOG(LogAssets, Error, "Importing failed: Null value returned");
				return false;
			}

			const std::vector<AssetLoadInfo> assetsToLoad(importedAssets->begin(), importedAssets->end());

			bool errorsEncountered = false;

			{ // Check to see if our user submitted multiple assets with the same name
				std::vector<std::string_view> duplicateNames{};
				for (size_t i = 0; i < assetsToLoad.size(); i++)
				{
					const std::string_view name = assetsToLoad[i].GetName();

					if (std::find(duplicateNames.begin(), duplicateNames.end(), name) != duplicateNames.end())
					{
						continue;
					}

					size_t numWithSameName{};
					for (size_t j = i + 1; j < assetsToLoad.size(); j++)
					{
						numWithSameName += assetsToLoad[i].GetName() == assetsToLoad[j].GetName();
					}

					if (numWithSameName != 0)
					{
						LOG(LogAssets, Error, "Importing failed: {} assets were imported with the name {}", numWithSameName, name);
						duplicateNames.push_back(name);
					}
				}

				errorsEncountered |= !duplicateNames.empty();
			}

			// Check if there are already assets with the submitted names,
			// and if we would be reimporting assets that are still referenced in memory
			for (const AssetLoadInfo& loadInfo : assetsToLoad)
			{
				const AssetInternal* const existingAssetWithSameName = TryGetAssetInternal(loadInfo.GetName(), loadInfo.GetAssetClass().GetTypeId());

				if (existingAssetWithSameName == nullptr)
				{
					continue;
				}

				if (!WasImportedFrom(*existingAssetWithSameName, path))
				{
					LOG(LogAssets, Error, "Importing failed: there is already an asset with the name {} (see {})",
						loadInfo.GetName(),
						existingAssetWithSameName->mFileOfOrigin.value_or("assets generated at runtime").string());

					errorsEncountered = true;
				}

				if (existingAssetWithSameName->mAsset.use_count() > 1)
				{
					LOG(LogAssets, Error, "Importing failed: Importing {} means replacing existing asset {}, but this asset is still referenced in memory {} time(s).",
						path.string(), existingAssetWithSameName->mMetaData.GetName(), existingAssetWithSameName->mAsset.use_count() - 1);
					errorsEncountered = true;
				}
			}

			if (errorsEncountered)
			{
				return false;
			}

			// Delete old files
			std::vector<std::pair<std::filesystem::path, std::string>> filesToRestoreInCaseOfErrors{};

			std::vector<WeakAsset<>> assetsToEraseEntirely{};

			for (auto& [key, assetInternal] : mAssets)
			{
				if (!WasImportedFrom(assetInternal, path))
				{
					continue;
				}

				// We can safely dereference the mFileOfOrigin,
				// because assets generated at runtime do not have an mImporterInfo.
				const std::filesystem::path& existingImportedAssetFile = *assetInternal.mFileOfOrigin;

				if (assetInternal.mMetaData.GetImporterInfo()->mWereEditsMadeAfterImporting)
				{
					LOG(LogAssets, Error, "Reimporting {} would undo all the changes made to {}. Delete the file {} before reimporting.",
						path.string(),
						assetInternal.mMetaData.mAssetName,
						existingImportedAssetFile.string());
					errorsEncountered = true;
					continue;
				}

				// Delete existing files that were generated the last time we imported this asset
				if (std::filesystem::exists(existingImportedAssetFile))
				{
					LOG(LogAssets, Message, "Deleting file {} created during previous importation", existingImportedAssetFile.string());

					std::ifstream fstream{ existingImportedAssetFile, std::ifstream::binary };

					if (fstream.is_open())
					{
						std::stringstream sstr{};
						sstr << fstream.rdbuf();
						fstream.close();

						filesToRestoreInCaseOfErrors.emplace_back(existingImportedAssetFile, sstr.str());
					}
					else
					{
						LOG(LogAssets, Warning, "Importing warning: Could not create a temporary backup of {}. If further errors are encountered, this file will not be restored.",
							existingImportedAssetFile.string());
					}

					std::error_code err{};
					if (!std::filesystem::remove(*assetInternal.mFileOfOrigin, err))
					{
						LOG(LogAssets, Error, "Importing failed: Could not delete file {} - {}", existingImportedAssetFile.string(), err.message());
						errorsEncountered = true;
					}
				}

				// During out previous importing, we created this asset. But now that we are importing again,
				// this asset was not produced. We need to remove this asset from our lookup
				if (std::find_if(assetsToLoad.begin(), assetsToLoad.end(),
					[keyCpy = key](const AssetLoadInfo& loadInfo)
					{
						return Name::HashString(loadInfo.GetName()) == keyCpy;
					}) == assetsToLoad.end())
				{
					if (assetInternal.mAsset.use_count() == 0)
					{
						assetsToEraseEntirely.push_back(assetInternal);
					}
					else
					{
						LOG(LogAssets, Error, "Importing failed: Importing {} means removing existing asset {}, but this asset is still referenced in memory {} time(s).",
							path.string(), assetInternal.mMetaData.GetName(), assetInternal.mAsset.use_count() - 1);
						errorsEncountered = true;
					}
				}
			}

			if (errorsEncountered)
			{
				for (const auto& [fileToRestore, content] : filesToRestoreInCaseOfErrors)
				{
					std::ofstream fstream{ fileToRestore, std::ofstream::binary };

					if (fstream.is_open())
					{
						fstream << content;
						LOG(LogAssets, Message, "Restored {}", fileToRestore.string());
					}
					else
					{
						LOG(LogAssets, Error, "Failed to restore {}", fileToRestore.string());
					}
				}
				return false;
			}

			for (WeakAsset<Asset>& assetToErase : assetsToEraseEntirely)
			{
				DeleteAsset(std::move(assetToErase));
			}

			// Finally, we can safely import
			std::filesystem::path outputDirectory = path.parent_path();

			if (assetsToLoad.size() > 1)
			{
				const std::filesystem::path folder = std::filesystem::path{ path }.replace_extension();

				if ((exists(folder)
					&& is_directory(folder))
					|| create_directory(folder))
				{
					outputDirectory = folder;
				}
			}

			for (size_t i = 0; i < importedAssets->size(); i++)
			{
				const AssetLoadInfo& loadInfo = assetsToLoad[i];
				const AssetSaveInfo& saveInfo = (*importedAssets)[i];

				AssetInternal* const existingAsset = TryGetAssetInternal(loadInfo.GetName(), loadInfo.GetAssetClass().GetTypeId());

				std::filesystem::path fileWeWantToSaveTo{};

				if (existingAsset != nullptr
					&& existingAsset->mFileOfOrigin.has_value())
				{
					fileWeWantToSaveTo = *existingAsset->mFileOfOrigin;

					if (&existingAsset->mMetaData.GetClass() != &loadInfo.GetAssetClass())
					{
						LOG(LogAssets, Warning, "Asset {} is reimported and changed from class {} to {}, existing WeakAssets could now be invalid",
							loadInfo.GetName(),
							existingAsset->mMetaData.GetClass().GetName(),
							loadInfo.GetAssetClass().GetName());
					}
				}
				else
				{
					std::string filename = loadInfo.GetName();

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

					fileWeWantToSaveTo = outputDirectory / filename.append(sAssetExtension);

					while (exists(fileWeWantToSaveTo))
					{
						fileWeWantToSaveTo.replace_filename(fileWeWantToSaveTo.filename().replace_extension().string().append("_Copy")).replace_extension(sAssetExtension);
					}
				}

				const bool success = saveInfo.SaveToFile(fileWeWantToSaveTo);

				if (!success)
				{
					LOG(LogAssets, Error, "Importing partially failed: Could not save {} to {}", loadInfo.GetName(), fileWeWantToSaveTo.string());
					continue;
				}

				LOG(LogAssets, Message, "Saved imported asset {} to {}", loadInfo.GetName(), fileWeWantToSaveTo.string());

				if (existingAsset != nullptr)
				{
					existingAsset->mMetaData = std::move(*loadInfo.mMetaData);
				}
				else
				{
					if (TryConstruct(fileWeWantToSaveTo, std::move(*loadInfo.mMetaData)) == nullptr)
					{
						LOG(LogAssets, Error, "Importing partially failed: Could not contruct asset from {}", fileWeWantToSaveTo.string());
					}
				}
			}

			LOG(LogAssets, Message, "Finished importing {}", path.string());

			return true;
		};

	if (refreshEngine)
	{
		Editor::Get().Refresh({ Editor::RefreshRequest::Volatile, importLambda });
	}
	else
	{
		importLambda();
	}*/