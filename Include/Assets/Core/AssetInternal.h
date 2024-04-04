#pragma once
#include "AssetFileMetaData.h"

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

		AssetFileMetaData mMetaData;

		// The .asset file. Is only nullopt if this
		// asset was generated at runtime, and no path
		// was provided by the user.
		std::optional<std::filesystem::path> mFileOfOrigin{};

		/*
		We're not using a weak_ptr; it'd be wasteful
		if we unloaded it, then have to load it back
		in later in the frame.

		If ref count == 1, the asset will be
		unloaded when UnloadAllUnusedAssets is
		called.
		*/
		std::shared_ptr<Asset> mAsset{};
	};
}
