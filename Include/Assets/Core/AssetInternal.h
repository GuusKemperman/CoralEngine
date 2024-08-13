#pragma once
#include <atomic>

#include "AssetFileMetaData.h"
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
		AssetInternal(AssetFileMetaData&& metaData, const std::optional<std::filesystem::path>& path);

		Asset* TryGetLoadedAsset();

		enum class RefCountType : bool { Strong, Weak };

		std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> mAsset{};

		std::array<std::atomic<uint32>, 2> mRefCounters{};
		std::atomic<bool> mHasBeenDereferencedSinceGarbageCollect{};
		std::mutex mAccessMutex{};

		AssetFileMetaData mMetaData;

		// The .asset file. Is only nullopt if this
		// asset was generated at runtime, and no path
		// was provided by the user.
		std::optional<std::filesystem::path> mFileOfOrigin{};

		// The .rename files that redirect to this asset.
		std::vector<std::filesystem::path> mOldNames{};
	};
}
