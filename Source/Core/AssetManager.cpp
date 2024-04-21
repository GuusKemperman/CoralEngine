#include "Precomp.h"
#include "Core/AssetManager.h"

#include "Core/FileIO.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaTools.h"
#include "Meta/MetaType.h"
#include "Core/Editor.h"
#include "Utilities/ClassVersion.h"
#include "Utilities/StringFunctions.h"

void CE::AssetManager::PostConstruct()
{
	mAssets.reserve(1024);

	OpenDirectory(FileIO::Get().GetPath(FileIO::Directory::EngineAssets, ""));
	OpenDirectory(FileIO::Get().GetPath(FileIO::Directory::GameAssets, ""));
}

void CE::AssetManager::OpenDirectory(const std::filesystem::path& directory)
{
	if (!std::filesystem::is_directory(directory))
	{
		LOG(LogAssets, Warning, "{} is not a directory", directory.string());
		return;
	}

	[[maybe_unused]] uint32 numOfAssetsFound{};

	for (const std::filesystem::directory_entry& dirEntry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (!std::filesystem::is_regular_file(dirEntry))
		{
			continue;
		}

		const std::filesystem::path& path = dirEntry.path();

		if (path.extension() == sAssetExtension)
		{
			numOfAssetsFound += OpenAsset(path);
		}
	}

	LOG(LogAssets, Verbose, "Finished indexing {}, found {} assets", directory.string(), numOfAssetsFound);
}

CE::Internal::AssetInternal* CE::AssetManager::TryGetAssetInternal(const Name key, const TypeId typeId)
{
	const auto it = mAssets.find(key.GetHash());

	if (it == mAssets.end()
		|| !it->second.mMetaData.GetClass().IsDerivedFrom(typeId))
	{
		return nullptr;
	}
	return &it->second;
}

CE::Internal::AssetInternal* CE::AssetManager::TryGetLoadedAssetInternal(const Name key, const TypeId typeId)
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

void CE::AssetManager::UnloadAllUnusedAssets()
{
	bool wereAnyUnloaded;

	do
	{
		wereAnyUnloaded = false;
		for (auto& [name, asset] : mAssets)
		{
			if (asset.mAsset == nullptr // Asset is already unloaded
				|| asset.mRefCounters[static_cast<int>(Internal::AssetInternal::RefCountType::Strong)] > 0 // Someone is holding a reference
				|| !asset.mFileOfOrigin.has_value()) // This asset was generated at runtime; if we unload it, we won't be able to load if back in again. 
			{
				continue;
			}

			Unload(asset);
			wereAnyUnloaded = true;
		}
	} while (wereAnyUnloaded); // We might've unloaded an asset that held onto the last reference of another asset
}

void CE::AssetManager::Load(Internal::AssetInternal& internalAsset)
{
	internalAsset.Load();
}

void CE::AssetManager::Unload(Internal::AssetInternal& asset)
{
	asset.UnLoad();
}

void CE::AssetManager::RenameAsset(WeakAssetHandle<> asset, std::string_view newName)
{
	auto renameLambda = [this, oldName = asset.GetMetaData().GetName(), newName = std::string{ newName }]()
		{
			WeakAssetHandle asset = TryGetWeakAsset(oldName);

			if (asset == nullptr)
			{
				return;
			}

			if (TryGetWeakAsset(newName) != nullptr)
			{
				LOG(LogAssets, Error, "Cannot rename asset {} to {}, there is already an asset with this name", oldName, newName);
				return;
			}

			AssetFileMetaData newMetaData = asset.GetMetaData();
			newMetaData.mAssetName = newName;

			Internal::AssetInternal* assetInternal = TryGetAssetInternal(asset.GetMetaData().GetName(), asset.GetMetaData().GetClass().GetTypeId());

			if (assetInternal == nullptr)
			{
				LOG(LogAssets, Error, "Cannot rename asset {} to {}, could not find internal asset", oldName, newName);
				return;
			}

			if (asset.GetFileOfOrigin().has_value())
			{
				const std::filesystem::path oldPath = *asset.GetFileOfOrigin();
				const std::filesystem::path newPath = std::filesystem::path{ oldPath }.replace_filename(newName).replace_extension(sAssetExtension);

				std::error_code err{};
				std::filesystem::rename(oldPath, newPath, err);

				if (err)
				{
					LOG(LogAssets, Error, "Cannot rename asset {}, could not rename file {} to {} - {}",
						asset.GetMetaData().GetName(),
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
						LOG(LogAssets, Error, "Cannot rename asset {}, could not open file {} for read", asset.GetMetaData().GetName(), newPath.string());
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

				assetInternal->mFileOfOrigin = newPath;
			}

			assetInternal->mMetaData = newMetaData;

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
	LOG(LogAssets, Warning, "Renaming asset {} without the editor. Existing weak assets to this asset will become dangling.", asset.GetMetaData().GetName());
	renameLambda();
#endif
}

void CE::AssetManager::DeleteAsset(WeakAssetHandle<>&& asset)
{
	auto deleteLambda = [this, assetName = asset.GetMetaData().GetName()]()
		{
			LOG(LogAssets, Verbose, "Asset {} will be erased. Any WeakAssets referencing it will now be dangling", assetName);

			WeakAssetHandle asset = TryGetWeakAsset(assetName);

			if (asset == nullptr)
			{
				return;
			}

			if (asset.GetFileOfOrigin().has_value())
			{
				std::error_code err{};
				std::filesystem::remove(*asset.GetFileOfOrigin(), err);

				if (err)
				{
					LOG(LogAssets, Error, "Asset {} was removed from the asset manager, but deleting {} failed ({}). Asset will be present again on next startup",
						assetName,
						asset.GetFileOfOrigin()->string(),
						err.message());
				}
			}

			mAssets.erase(Name::HashString(asset.GetMetaData().GetName()));
		};

#ifdef EDITOR
	Editor::Get().Refresh({ Editor::RefreshRequest::Volatile, deleteLambda });
#else
	LOG(LogAssets, Warning, "Deleting asset {} without the editor. Existing weak assets to this asset will become dangling.", asset.GetMetaData().GetName());
	deleteLambda();
#endif
}

bool CE::AssetManager::MoveAsset(WeakAssetHandle<> asset, const std::filesystem::path& toLocation)
{
	if (!asset.GetFileOfOrigin().has_value())
	{
		LOG(LogAssets, Error, "Failed to move asset {} to {}: This asset was generated at runtime, there is no original file to copy from.",
			asset.GetMetaData().GetAssetVersion(),
			toLocation.string());
		return false;
	}

	if (toLocation.extension() != sAssetExtension)
	{
		LOG(LogAssets, Error, "Failed to move asset {} to {}: The destination extension was not {}.",
			asset.GetMetaData().GetAssetVersion(),
			toLocation.string(),
			sAssetExtension);
		return false;
	}

	Internal::AssetInternal* assetInternal = TryGetAssetInternal(asset.GetMetaData().GetName(), asset.GetMetaData().GetClass().GetTypeId());

	if (assetInternal == nullptr)
	{
		LOG(LogAssets, Error, "Failed to move asset {} to {}: Could not find internal asset",
			asset.GetMetaData().GetAssetVersion(),
			toLocation.string());
		return false;
	}

	std::filesystem::path& assetFile = *assetInternal->mFileOfOrigin;

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

CE::WeakAssetHandle<> CE::AssetManager::Duplicate(WeakAssetHandle<> asset, const std::filesystem::path& copyPath)
{
	if (!asset.GetFileOfOrigin().has_value())
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, as it did not come from a file", asset.GetMetaData().GetName());
		return nullptr;
	}

	if (std::filesystem::exists(copyPath))
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, as there is already a file {}", asset.GetMetaData().GetName(), copyPath.string());
		return nullptr;
	}

	if (copyPath.extension() != sAssetExtension)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {} to {}, the extension was not {}", asset.GetMetaData().GetName(), copyPath.string(), sAssetExtension);
		return nullptr;
	}

	const std::string copyName = copyPath.filename().replace_extension().string();

	if (TryGetWeakAsset(copyName) != nullptr)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, as there is already an asset with this name", asset.GetMetaData().GetName());
		return nullptr;
	}

	std::string fileContents{};

	{
		std::ifstream file{ *asset.GetFileOfOrigin(), std::ifstream::binary };

		if (!file.is_open())
		{
			LOG(LogAssets, Error, "Cannot duplicate asset {}, could not open file {}", asset.GetMetaData().GetName(), asset.GetFileOfOrigin()->string());
			return nullptr;
		}

		(void)AssetFileMetaData::ReadMetaData(file);

		fileContents = StringFunctions::StreamToString(file);
	}


	AssetFileMetaData newMetaData{ copyName, asset.GetMetaData().GetClass(), asset.GetMetaData().GetAssetVersion() };

	{
		std::filesystem::create_directories(copyPath.parent_path());
		std::ofstream file{ copyPath, std::ofstream::binary };

		if (!file.is_open())
		{
			LOG(LogAssets, Error, "Cannot duplicate asset {}, could not open file {}", asset.GetMetaData().GetName(), copyPath.string());
			return nullptr;
		}

		newMetaData.WriteMetaData(file);
		file.write(fileContents.c_str(), fileContents.size());
	}

	const Internal::AssetInternal* const constructedAsset = TryConstruct(copyPath.string());

	if (constructedAsset == nullptr)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, could not construct asset. File was duplicated however, see {}", asset.GetMetaData().GetName(), copyPath.string());
		return nullptr;
	}

	return TryGetWeakAsset(copyName);
}

CE::WeakAssetHandle<> CE::AssetManager::NewAsset(const MetaType& assetClass, const std::filesystem::path& path)
{
	const std::string assetName = path.filename().replace_extension().string();

	if (AssetManager::Get().TryGetWeakAsset(Name{ assetName }) != nullptr)
	{
		LOG(LogAssets, Error, "Cannot create new asset {}, there is already an asset with this name", assetName);
		return nullptr;
	}

	if (std::filesystem::exists(path))
	{
		LOG(LogAssets, Error, "Cannot create new asset {} at {}, there is already a file at this location", assetName, path.string());
		return nullptr;
	}

	if (path.extension() != sAssetExtension)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {} to {}, the extension was not {}", assetName, path.string(), sAssetExtension);
		return nullptr;
	}

	const std::string_view strView{ assetName };
	FuncResult constructResult = assetClass.Construct(strView);

	if (constructResult.HasError())
	{
		LOG(LogAssets, Error, "Failed to create new asset of type {} - {}", assetClass.GetName(), constructResult.Error());
		return nullptr;
	}

	const Asset* const asset = constructResult.GetReturnValue().As<Asset>();

	if (asset == nullptr)
	{
		LOG(LogAssets, Error, "Failed to create new asset of type {} - Construct result was not an asset", assetClass.GetName());
		return nullptr;
	}

	const AssetSaveInfo saveInfo = asset->Save();
	const bool success = saveInfo.SaveToFile(path);

	if (!success)
	{
		LOG(LogAssets, Error, "Failed to create new asset, the file {} could not be saved to", path.string());
		return nullptr;
	}

	const Internal::AssetInternal* const constructedAsset = TryConstruct(path);

	if (constructedAsset == nullptr)
	{
		LOG(LogAssets, Error, "Cannot duplicate asset {}, could not construct asset. File was duplicated however, see {}", assetName, path.string());
		return nullptr;
	}

	return TryGetWeakAsset(assetName);
}

CE::WeakAssetHandle<> CE::AssetManager::OpenAsset(const std::filesystem::path& path)
{
	return { TryConstruct(path) };
}

CE::Internal::AssetInternal* CE::AssetManager::TryConstruct(const std::filesystem::path& path)
{
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

CE::Internal::AssetInternal* CE::AssetManager::TryConstruct(const std::optional<std::filesystem::path>& path, AssetFileMetaData metaData)
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

	Internal::AssetInternal& constructedAssetInternal = emplaceResult.first->second;

#ifdef LOGGING_ENABLED
	const uint32 currentVersion = GetClassVersion(constructedAssetInternal.mMetaData.GetClass());
	if (constructedAssetInternal.mMetaData.mAssetVersion != currentVersion)
	{
		LOG(LogAssets, Verbose, "Asset {} is out of date: version is {} (current is {}). If the loader still supports this version, you have nothing to worry about.",
			constructedAssetInternal.mMetaData.mAssetName,
			constructedAssetInternal.mMetaData.mAssetVersion,
			currentVersion);
	}
#endif // LOGGING_ENABLED

	return &constructedAssetInternal;
}
