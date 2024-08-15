#pragma once
#include <atomic>

#include "AssetMetaData.h"
#include "Utilities/MemFunctions.h"

namespace CE
{
	class Asset;
}

namespace CE::Internal
{
	// A potentially unloaded asset.
	struct AssetInternal
	{
		AssetInternal(AssetMetaData metaData, std::optional<std::filesystem::path> path);
		AssetInternal(std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> asset);

		Asset* TryGetLoadedAsset();

		enum class RefCountType : bool { Strong, Weak };

		std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> mAsset{};

		// Data that is used to hint to the asset manager
		// whether the asset is in use or if it can be offloaded.
		std::array<std::atomic<uint32>, 2> mRefCounters{};
		std::atomic<bool> mHasBeenDereferencedSinceGarbageCollect{};

		// Used for reading/writing to mAsset
		std::mutex mAccessMutex{};

		// Certain data can be accessed without having
		// to load the entire asset in
		AssetMetaData mMetaData;

		// The .asset file. Is only nullopt if this
		// asset was generated at runtime, and no path
		// was provided by the user.
		std::optional<std::filesystem::path> mFileOfOrigin{};

		// The .rename files that redirect to this asset.
		std::vector<std::filesystem::path> mOldNames{};
	};
}
