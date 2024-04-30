#pragma once
#include <atomic>

#include "AssetFileMetaData.h"
#include "Utilities/MemFunctions.h"
#include "Utilities/ASync.h"

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

		// Will return the Asset, if it has finished loading.
		Asset* TryGet();

		// Will finish loading if we havent already and return the asset
		Asset& Get();

		void StartLoadingIfNotStarted();

		void UnloadIfLoaded();

		bool IsLoaded() const;

		enum class RefCountType : bool { Strong, Weak };

		std::array<std::atomic<uint32>, 2> mRefCounters{};

		ASyncFuture<std::unique_ptr<Asset, InPlaceDeleter<Asset, true>>> mAsset{};

		AssetFileMetaData mMetaData;

		// The .asset file. Is only nullopt if this
		// asset was generated at runtime, and no path
		// was provided by the user.
		std::optional<std::filesystem::path> mFileOfOrigin{};

		// The .rename files that redirect to this asset.
		std::vector<std::filesystem::path> mOldNames{};

		std::mutex mLoadUnloadMutex{};
	};
}
