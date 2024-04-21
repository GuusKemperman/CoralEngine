#pragma once
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

		void Load();
		void UnLoad();

		enum class RefCountType : bool { Strong, Weak };

		std::array<std::atomic<uint32>, 2> mRefCounters{};

		AssetFileMetaData mMetaData;

		// The .asset file. Is only nullopt if this
		// asset was generated at runtime, and no path
		// was provided by the user.
		std::optional<std::filesystem::path> mFileOfOrigin{};

		std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> mAsset{};

	};
}
