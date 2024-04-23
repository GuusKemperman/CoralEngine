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
	mLookUp.reserve(1024);

	OpenDirectory(FileIO::Get().GetPath(FileIO::Directory::EngineAssets, ""));
	OpenDirectory(FileIO::Get().GetPath(FileIO::Directory::GameAssets, ""));
}

CE::AssetManager::~AssetManager()
{
	for (Internal::AssetInternal& assetInternal : mAssets)
	{
		if (assetInternal.mAsset != nullptr)
		{
			assetInternal.UnLoad();
		}
	}
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
	const auto it = mLookUp.find(key.GetHash());

	if (it == mLookUp.end()
		|| !it->second.get().mMetaData.GetClass().IsDerivedFrom(typeId))
	{
		return nullptr;
	}
	return &it->second.get();
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
		internalAsset->Load();
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
		for (Internal::AssetInternal& asset : mAssets)
		{
			if (asset.mAsset == nullptr // Asset is already unloaded
				|| asset.mRefCounters[static_cast<int>(Internal::AssetInternal::RefCountType::Strong)] > 0 // Someone is holding a reference
				|| !asset.mFileOfOrigin.has_value()) // This asset was generated at runtime; if we unload it, we won't be able to load if back in again. 
			{
				continue;
			}

			asset.UnLoad();
			wereAnyUnloaded = true;
		}
	} while (wereAnyUnloaded); // We might've unloaded an asset that held onto the last reference of another asset
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

			mLookUp.erase(Name::HashString(oldName));
			auto emplaceResult = mLookUp.emplace(Name::HashString(newName), *assetInternal);

			if (!emplaceResult.second)
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
			const Internal::AssetInternal* const asset = TryGetAssetInternal(assetName, MakeTypeId<Asset>());

			if (asset == nullptr)
			{
				LOG(LogAssets, Warning, "Could not delete asset {}, it seems to not be present anymore. Maybe asset was already deleted?",
					assetName);
				return;
			}

			const uint32 numOfReferences = asset->mRefCounters[0] + asset->mRefCounters[1];
			if (numOfReferences != 0)
			{
				LOG(LogAssets, Error, "Could not safely delete asset {}, as it was still referenced {} time(s).", 
					assetName,
					numOfReferences);
				return;
			}

			if (asset->mFileOfOrigin.has_value())
			{
				std::error_code err{};
				std::filesystem::remove(*asset->mFileOfOrigin, err);

				if (err)
				{
					LOG(LogAssets, Error, "Asset {} was removed from the asset manager, but deleting {} failed ({}). Asset will be present again on next startup",
						assetName,
						asset->mFileOfOrigin->string(),
						err.message());
				}
			}

			mLookUp.erase(Name::HashString(assetName));
			mAssets.remove_if([&assetName](const Internal::AssetInternal& asset)
				{
					return asset.mMetaData.GetName() == assetName;
				});
		};

#ifdef EDITOR
	if (asset.GetNumberOfStrongReferences() != 0
		|| asset.GetNumberOfSoftReferences() != 1)
	{
		Editor::Get().Refresh({ Editor::RefreshRequest::Volatile, deleteLambda });
		return;
	}
#else
	LOG(LogAssets, Warning, "Deleting asset {} without the editor. Existing weak assets to this asset will become dangling.", asset.GetMetaData().GetName());
#endif

	{
		// Remove the last reference
		[[maybe_unused]] WeakAssetHandle<> tmp = std::move(asset);
	}

	deleteLambda();
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

	std::optional<AssetFileMetaData> metaData = AssetFileMetaData::ReadMetaData(istream);

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

	Internal::AssetInternal& assetInternal = mAssets.emplace_front(std::move(metaData), path);
	
	const auto emplaceResult = mLookUp.emplace(Name::HashString(assetInternal.mMetaData.GetName()), assetInternal);

	if (!emplaceResult.second)
	{
		LOG(LogAssets, Error, "Failed to construct asset {}: there is already an asset with the name {}, from {}",
			path.value_or("Generated at runtime").string(), assetInternal.mMetaData.GetName(), assetInternal.mFileOfOrigin.value_or("Generated at runtime").string());
		mAssets.pop_front();
		return nullptr;
	}

#ifdef LOGGING_ENABLED
	const uint32 currentVersion = GetClassVersion(assetInternal.mMetaData.GetClass());
	if (assetInternal.mMetaData.mAssetVersion != currentVersion)
	{
		LOG(LogAssets, Verbose, "Asset {} is out of date: version is {} (current is {}). If the loader still supports this version, you have nothing to worry about.",
			assetInternal.mMetaData.mAssetName,
			assetInternal.mMetaData.mAssetVersion,
			currentVersion);
	}
#endif // LOGGING_ENABLED

	return &assetInternal;
}
