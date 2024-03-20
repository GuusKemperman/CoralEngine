#include "Precomp.h"
#include "Core/AssetManager.h"

#include "Core/FileIO.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Assets/Importers/Importer.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaTools.h"
#include "Meta/MetaType.h"
#include "Utilities/ClassVersion.h"
#include "Core/Editor.h"
#include "Utilities/StringFunctions.h"

Engine::AssetManager::AssetManager()
{
#ifdef EDITOR
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

					if (extension == sAssetExtension)
					{
						LOG(LogAssets, Warning, "Importer {} has invalid extension {}: extensions cannot be the same as the asset extension \"{}\"",
							derived.GetName(),
							extension.string(),
							sAssetExtension);
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
#endif // EDITOR
}

Engine::AssetManager::~AssetManager() = default;

void Engine::AssetManager::PostConstruct()
{
	OpenDirectory(FileIO::Get().GetPath(FileIO::Directory::EngineAssets, ""));
	OpenDirectory(FileIO::Get().GetPath(FileIO::Directory::GameAssets, ""));
}

void Engine::AssetManager::OpenDirectory(const std::filesystem::path& directory)
{
	if (!is_directory(directory))
	{
		LOG(LogAssets, Warning, "{} is not a directory", directory.string());
		return;
	}

#ifdef EDITOR
	std::vector<std::filesystem::path> importableAssets{};
#endif // EDITOR

	for (const std::filesystem::directory_entry& dirEntry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (!is_regular_file(dirEntry))
		{
			continue;
		}

		const std::filesystem::path& path = dirEntry.path();
		const std::string extension = path.extension().string();

		if (extension == sAssetExtension)
		{
			OpenAsset(path);
			continue;
		}

#ifdef EDITOR
		importableAssets.push_back(path);
#endif // EDITOR
	}

#ifdef EDITOR
	for (const std::filesystem::path& importableAsset : importableAssets)
	{
		const auto [importerTypeId, importer] = TryGetImporterForExtension(importableAsset.extension());

		if (importer == nullptr)
		{
			continue;
		}

		const std::filesystem::file_time_type importableAssetLastWriteTime = last_write_time(importableAsset);

		bool wasPreviouslyImported = false;

		for (const auto& [key, assetInternal] : mAssets)
		{
			if (!WasImportedFrom(assetInternal, importableAsset))
			{
				continue;
			}

			wasPreviouslyImported = true;

			const uint32 assetImporterVersion = assetInternal.mMetaData.mImporterInfo->mImporterVersion;

			const MetaType* const importerType = MetaManager::Get().TryGetType(importerTypeId);
			ASSERT(importerType != nullptr);

			if (assetImporterVersion != GetClassVersion(*importerType))
			{
				LOG(LogAssets, Message, "Asset {} was imported with an older version of the importer ({}). Reimporting...",
					importableAsset.string(),
					assetImporterVersion);
				ImportInternal(importableAsset, false);
				break;
			}

			const uint32 currentAssetVersion = GetClassVersion(assetInternal.mMetaData.GetClass());

			if (assetInternal.mMetaData.mAssetVersion != currentAssetVersion)
			{
				LOG(LogAssets, Message, "Asset {} is out-of-date, version is {} (current is {}). The asset will be re-imported from {}",
					assetInternal.mMetaData.GetName(),
					assetInternal.mMetaData.mAssetVersion,
					currentAssetVersion,
					importableAsset.string());
				ImportInternal(importableAsset, false);
				break;
			}

			if (std::filesystem::last_write_time(*assetInternal.mFileOfOrigin) < importableAssetLastWriteTime)
			{
				LOG(LogAssets, Message, "Changes to {} detected. Reimporting...",
					importableAsset.string());
				ImportInternal(importableAsset, false);
				break;
			}
		}

		if (!wasPreviouslyImported)
		{
			LOG(LogAssets, Message, "New content detected at {}. Importing...", importableAsset.string());
			ImportInternal(importableAsset, false);
		}
	}
#endif // EDITOR

	LOG(LogAssets, Verbose, "Finished constructing assets");
}

Engine::AssetManager::AssetInternal::AssetInternal(AssetFileMetaData&& metaData, const std::optional<std::filesystem::path>& path) :
	mMetaData(std::move(metaData)),
	mFileOfOrigin(path)
{
}

Engine::AssetManager::AssetInternal* Engine::AssetManager::TryGetAssetInternal(const Name key, const TypeId typeId)
{
	const auto it = mAssets.find(key.GetHash());

	if (it == mAssets.end()
		|| !it->second.mMetaData.GetClass().IsDerivedFrom(typeId))
	{
		return nullptr;
	}
	return &it->second;
}

Engine::AssetManager::AssetInternal* Engine::AssetManager::TryGetLoadedAssetInternal(const Name key, const TypeId typeId)
{
	auto* internalAsset = TryGetAssetInternal(key, typeId);

	if (internalAsset == nullptr)
	{
		return nullptr;
	}

	if (internalAsset->mAsset == nullptr)
	{
		Load(*internalAsset);
		ASSERT(internalAsset->mAsset != nullptr);
	}
	return internalAsset;
}

void Engine::AssetManager::Unload(AssetInternal& asset)
{
	if (asset.mAsset != nullptr)
	{
		LOG(LogAssets, Verbose, "Unloading {}", asset.mMetaData.mAssetName);
		asset.mAsset.reset();
	}
	else
	{
		LOG(LogAssets, Verbose, "Asset {} is already unloaded", asset.mMetaData.mAssetName);
	}
}

void Engine::AssetManager::UnloadAllUnusedAssets()
{
	bool wereAnyUnloaded;

	do
	{
		wereAnyUnloaded = false;
		for (auto& [name, asset] : mAssets)
		{
			if (asset.mAsset == nullptr // Asset is already unloaded
				|| asset.mAsset.use_count() > 1 // We hold a reference, and atleast in one other place someone is holding a reference as well.
				|| !asset.mFileOfOrigin.has_value()) // This asset was generated at runtime; if we unload it, we won't be able to load if back in again. 
			{
				continue;
			}

			Unload(asset);
			wereAnyUnloaded = true;
		}
	} while (wereAnyUnloaded); // We might've unloaded an asset that held onto the last reference of another asset
}

std::vector<Engine::WeakAsset<Engine::Asset>> Engine::AssetManager::GetAllAssets()
{
	std::vector<WeakAsset<Asset>> returnValue{};
	returnValue.reserve(mAssets.size());

	for (auto& [name, assetInternal] : mAssets)
	{
		returnValue.push_back(WeakAsset<Asset>{assetInternal});
	}

	return returnValue;
}

void Engine::AssetManager::Load(AssetInternal& internalAsset)
{
	LOG(LogAssets, Verbose, "Asset manager is loading {}", internalAsset.mMetaData.mAssetName);

	auto& asset = internalAsset.mAsset;

	if (asset != nullptr)
	{
		LOG(LogAssets, Warning, "Attempting to load {} twice", internalAsset.mMetaData.mAssetName);
		return;
	}

	ASSERT_LOG(internalAsset.mFileOfOrigin.has_value(), "Attempted to load {}, but this asset was generated at runtime and should not have been unloaded to begin with", internalAsset.mAsset->GetName());

	AssetLoadInfo loadInfo{ *internalAsset.mFileOfOrigin };

	FuncResult constructResult = internalAsset.mMetaData.mClass.get().Construct(loadInfo);

	if (constructResult.HasError())
	{
		LOG(LogAssets, Error, "Asset {} could not be constructed. Does it have a constructor that takes a LoadInfo&, and was this constructor reflected in your reflect function? {}",
			internalAsset.mMetaData.mClass.get().GetName(),
			constructResult.Error());
		return;
	}

	asset = MakeShared<Asset>(std::move(constructResult.GetReturnValue()));
}

#ifdef EDITOR
bool Engine::AssetManager::WasImportedFrom(const AssetInternal& asset, const std::filesystem::path& file)
{
	return asset.mMetaData.mImporterInfo.has_value()
		// We only look at the filename, as the full path may be different.
		// For example the EngineAssets folder may be moved relative to the
		// working environment, or may always be different for several projects.
		//
		// So the relative path can change, but so can the absolute path;
		// one person might save their engine on the D: drive, while someone
		// else might save it in C:/Projects/Repos.
		//
		// Since neither option is ideal, we only look at the filename.
		&& asset.mMetaData.mImporterInfo->mImportedFile.filename() == file.filename();
}

void Engine::AssetManager::Import(const std::filesystem::path& path)
{
	ImportInternal(path, true);
}

void Engine::AssetManager::ImportInternal(const std::filesystem::path& path, bool refreshEngine)
{
	auto importLambda = [this, path]
		{
			const auto [importerTypeId, importer] = TryGetImporterForExtension(path.extension());

			LOG(LogAssets, Message, "Importing {}", path.string());

			if (importer == nullptr)
			{
				LOG(LogAssets, Error, "No importer that can import {}.", path.string());
				return false;
			}

			// Collect new files
			std::optional<std::vector<ImportedAsset>> importedAssets = importer->Import(path);

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
				else if (existingAssetWithSameName->mAsset.use_count() > 1)
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
	}
}


std::pair<Engine::TypeId, const Engine::Importer*> Engine::AssetManager::TryGetImporterForExtension(const std::filesystem::path& extension) const
{
	for (const auto& [typeId, importer] : mImporters)
	{
		std::vector<std::filesystem::path> canImport = importer->CanImportExtensions();
		auto it = std::find(canImport.begin(), canImport.end(), extension);

		if (it != canImport.end())
		{
			return { typeId, importer.get() };
		}
	}

	return { 0, nullptr };
}
#endif // EDITOR

void Engine::AssetManager::RenameAsset(WeakAsset<> asset, std::string_view newName)
{
	auto renameLambda = [this, oldName = asset.GetName(), newName = std::string{ newName }]()
		{
			std::optional<WeakAsset<Asset>> asset = TryGetWeakAsset(oldName);

			if (!asset.has_value())
			{
				return;
			}

			if (TryGetWeakAsset(newName).has_value())
			{
				LOG(LogAssets, Error, "Cannot rename asset {} to {}, there is already an asset with this name", oldName, newName);
				return;
			}


			AssetFileMetaData newMetaData = asset->mAssetInternal.get().mMetaData;
			newMetaData.mAssetName = newName;

			if (asset->GetFileOfOrigin().has_value())
			{
				const std::filesystem::path oldPath = *asset->mAssetInternal.get().mFileOfOrigin;
				const std::filesystem::path newPath = std::filesystem::path{ oldPath }.replace_filename(newName).replace_extension(sAssetExtension);

				std::error_code err{};
				std::filesystem::rename(oldPath, newPath, err);

				if (err)
				{
					LOG(LogAssets, Error, "Cannot rename asset {}, could not rename file {} to {} - {}",
						asset->GetName(),
						oldPath.string(),
						newPath.string(),
						err.message());
					return;
				}

				std::string fileContents{};

				{
					std::ifstream file{ newPath, std::ifstream::binary };

					if (!file.is_open())
					{
						LOG(LogAssets, Error, "Cannot rename asset {}, could not open file {} for read", asset->GetName(), newPath.string());
						return;
					}

					(void)AssetFileMetaData::ReadMetaData(file);

					fileContents = StringFunctions::StreamToString(file);
				}


				{
					std::ofstream file{ newPath, std::ofstream::binary };

					if (!file.is_open())
					{
						LOG(LogAssets, Error, "Cannot rename asset {}, could not open file {} for writing", oldName, newPath.string());
						return;
					}

					newMetaData.WriteMetaData(file);
					file.write(fileContents.c_str(), fileContents.size());
				}

				asset->mAssetInternal.get().mFileOfOrigin = newPath;
			}

			asset->mAssetInternal.get().mMetaData = newMetaData;
			auto extractedAsset = mAssets.extract(Name::HashString(oldName));
			extractedAsset.key() = Name::HashString(newName);
			auto insertResult = mAssets.insert(std::move(extractedAsset));

			if (!insertResult.inserted)
			{
				LOG(LogAssets, Error, "Cannot rename asset {}, inesrtion somehow failed. Assets is deleted from memory, but still exists on file", oldName);
			}
		};

#ifdef EDITOR
	Editor::Get().Refresh({ Editor::RefreshRequest::Volatile, renameLambda });
#else
	LOG(LogAssets, Warning, "Renaming asset {} without the editor. Existing weak assets to this asset will become dangling.", asset.GetName());
	renameLambda();
#endif
}

void Engine::AssetManager::DeleteAsset(WeakAsset<Asset>&& asset)
{
	auto deleteLambda = [this, assetName = asset.GetName()]()
		{
			LOG(LogAssets, Verbose, "Asset {} will be erased. Any WeakAssets referencing it will now be dangling", assetName);

			std::optional<WeakAsset<Asset>> asset = TryGetWeakAsset(assetName);

			if (!asset.has_value())
			{
				return;
			}


			if (asset->GetFileOfOrigin().has_value())
			{
				std::error_code err{};
				std::filesystem::remove(*asset->GetFileOfOrigin(), err);

				if (err)
				{
					LOG(LogAssets, Error, "Asset {} was removed from the asset manager, but deleting {} failed ({}). Asset will be present again on next startup",
						assetName,
						asset->GetFileOfOrigin()->string(),
						err.message());
				}
			}

			mAssets.erase(Name::HashString(asset->GetName()));
		};

#ifdef EDITOR
	Editor::Get().Refresh({ Editor::RefreshRequest::Volatile, deleteLambda });
#else
	LOG(LogAssets, Warning, "Deleting asset {} without the editor. Existing weak assets to this asset will become dangling.", asset.GetName());
	deleteLambda();
#endif


}

bool Engine::AssetManager::MoveAsset(WeakAsset<Asset> asset, const std::filesystem::path& toLocation)
{
	if (!asset.mAssetInternal.get().mFileOfOrigin.has_value())
	{
		LOG(LogAssets, Error, "Failed to move asset {} to {}: This asset was generated at runtime, there is no original file to copy from.",
			asset.GetVersion(),
			toLocation.string());
		return false;
	}

	if (toLocation.extension() != sAssetExtension)
	{
		LOG(LogAssets, Error, "Failed to move asset {} to {}: The destination extension was not {}.",
			asset.GetVersion(),
			toLocation.string(),
			sAssetExtension);
		return false;
	}

	std::filesystem::path& assetFile = *asset.mAssetInternal.get().mFileOfOrigin;

	LOG(LogAssets, Message, "Moving file from {} to {}", assetFile.string(), toLocation.string());

	std::filesystem::create_directories(toLocation.parent_path());

	std::error_code err{};
	std::filesystem::copy(assetFile, toLocation, err);

	if (err)
	{
		LOG(LogAssets, Error, "Failed to move asset from {} to {}: Copying failed - {}",
			assetFile.string(),
			toLocation.string(),
			err.message());
		return false;
	}

	std::filesystem::remove(assetFile, err);
	if (err)
	{
		LOG(LogAssets, Warning, "After moving asset from {} to {}: Could not delete original file - {}",
			assetFile.string(),
			toLocation.string(),
			err.message());
	}

	assetFile = toLocation;

	return true;
}

std::optional<Engine::WeakAsset<Engine::Asset>> Engine::AssetManager::Duplicate(WeakAsset<Engine::Asset> asset, const std::filesystem::path& copyPath)
{
	if (!asset.GetFileOfOrigin().has_value())
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, as it did not come from a file", asset.GetName());
		return std::nullopt;
	}

	if (std::filesystem::exists(copyPath))
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, as there is already a file {}", asset.GetName(), copyPath.string());
		return std::nullopt;
	}

	if (copyPath.extension() != sAssetExtension)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {} to {}, the extension was not {}", asset.GetName(), copyPath.string(), sAssetExtension);
		return std::nullopt;
	}

	const std::string copyName = copyPath.filename().replace_extension().string();

	if (TryGetWeakAsset(copyName).has_value())
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, as there is already an asset with this name", asset.GetName());
		return std::nullopt;
	}

	std::string fileContents{};

	{
		std::ifstream file{ *asset.mAssetInternal.get().mFileOfOrigin, std::ifstream::binary };

		if (!file.is_open())
		{
			LOG(LogAssets, Error, "Cannot duplicate asset {}, could not open file {}", asset.GetName(), asset.mAssetInternal.get().mFileOfOrigin->string());
			return std::nullopt;
		}

		(void)AssetFileMetaData::ReadMetaData(file);

		fileContents = StringFunctions::StreamToString(file);
	}


	AssetFileMetaData newMetaData{ copyName, asset.GetAssetClass(), asset.GetVersion() };

	{
		std::ofstream file{ copyPath, std::ofstream::binary };

		if (!file.is_open())
		{
			LOG(LogAssets, Error, "Cannot duplicate asset {}, could not open file {}", asset.GetName(), copyPath.string());
			return std::nullopt;
		}

		newMetaData.WriteMetaData(file);
		file.write(fileContents.c_str(), fileContents.size());
	}

	const AssetInternal* const constructedAsset = TryConstruct(copyPath.string());

	if (constructedAsset == nullptr)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, could not construct asset. File was duplicated however, see {}", asset.GetName(), copyPath.string());
		return std::nullopt;
	}

	return TryGetWeakAsset(copyName);
}

std::optional<Engine::WeakAsset<>> Engine::AssetManager::NewAsset(const MetaType& assetClass,
	const std::filesystem::path& path)
{
	const std::string assetName = path.filename().replace_extension().string();

	if (AssetManager::Get().TryGetWeakAsset(Name{ assetName }).has_value())
	{
		LOG(LogAssets, Error, "Cannot create new asset {}, there is already an asset with this name", assetName);
		return std::nullopt;
	}

	if (std::filesystem::exists(path))
	{
		LOG(LogAssets, Error, "Cannot create new asset {} at {}, there is already a file at this location", assetName, path.string());
		return std::nullopt;
	}

	if (path.extension() != sAssetExtension)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {} to {}, the extension was not {}", assetName, path.string(), sAssetExtension);
		return std::nullopt;
	}

	const std::string_view strView{ assetName };
	FuncResult constructResult = assetClass.Construct(strView);

	if (constructResult.HasError())
	{
		LOG(LogAssets, Error, "Failed to create new asset of type {} - {}", assetClass.GetName(), constructResult.Error());
		return std::nullopt;
	}

	const Asset* const asset = constructResult.GetReturnValue().As<Asset>();

	if (asset == nullptr)
	{
		LOG(LogAssets, Error, "Failed to create new asset of type {} - Construct result was not an asset", assetClass.GetName());
		return std::nullopt;
	}

	const AssetSaveInfo saveInfo = asset->Save();
	const bool success = saveInfo.SaveToFile(path);

	if (!success)
	{
		LOG(LogAssets, Error, "Failed to create new asset, the file {} could not be saved to", path.string());
		return std::nullopt;
	}

	const AssetInternal* const constructedAsset = TryConstruct(path);

	if (constructedAsset == nullptr)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, could not construct asset. File was duplicated however, see {}", assetName, path.string());
		return std::nullopt;
	}

	return TryGetWeakAsset(assetName);
}

std::optional<Engine::WeakAsset<Engine::Asset>> Engine::AssetManager::OpenAsset(const std::filesystem::path& path)
{
	AssetInternal* const internalAsset = TryConstruct(path);

	if (internalAsset == nullptr)
	{
		return std::nullopt;
	}
	return WeakAsset<Asset>{ *internalAsset };
}

Engine::AssetManager::AssetInternal* Engine::AssetManager::TryConstruct(const std::filesystem::path& path)
{
	LOG(LogAssets, Verbose, "Constructing asset from {}", path.string());

	if (path.extension() != sAssetExtension)
	{
		LOG(LogAssets, Error, "Failed to construct asset {}: Expected extension {}, but extension was {}.",
			path.string(),
			sAssetExtension,
			path.extension().string());
		return nullptr;
	}

	std::ifstream istream{ path, std::ifstream::binary };

	if (!istream.is_open())
	{
		LOG(LogAssets, Warning, "Failed to construct asset {}: Could not open file", path.string());
		return nullptr;
	}

	const std::optional<AssetFileMetaData> metaData = AssetFileMetaData::ReadMetaData(istream);

	if (!metaData.has_value())
	{
		LOG(LogAssets, Warning, "Failed to construct asset {}: metadata was invalid", path.string());
		return nullptr;
	}

	return TryConstruct(path, std::move(*metaData));
}

Engine::AssetManager::AssetInternal* Engine::AssetManager::TryConstruct(const std::optional<std::filesystem::path>& path, AssetFileMetaData metaData)
{
	if (path.has_value()
		&& path->extension() != sAssetExtension)
	{
		LOG(LogAssets, Warning, "Expected {}, but extension was {}.", sAssetExtension, path->extension().string());
	}

	const auto emplaceResult = mAssets.try_emplace(Name::HashString(metaData.mAssetName),
		std::move(metaData),
		path);

	if (!emplaceResult.second)
	{
		LOG(LogAssets, Warning, "Failed to construct asset {}: there is already an asset with the name {}, from {}. Returning existing asset.",
			path.value_or("Generated at runtime").string(), emplaceResult.first->second.mMetaData.GetName(), emplaceResult.first->second.mFileOfOrigin.value_or("Generated at runtime").string());
	}

	AssetInternal& constructedAssetInternal = emplaceResult.first->second;

	const uint32 currentVersion = GetClassVersion(constructedAssetInternal.mMetaData.GetClass());
	if (constructedAssetInternal.mMetaData.mAssetVersion != currentVersion)
	{
		LOG(LogAssets, Message, "Asset {} is out of date: version is {} (current is {}). If the loader still supports this version, you have nothing to worry about.",
			constructedAssetInternal.mMetaData.mAssetName,
			constructedAssetInternal.mMetaData.mAssetVersion,
			currentVersion);
	}

	return &constructedAssetInternal;
}
