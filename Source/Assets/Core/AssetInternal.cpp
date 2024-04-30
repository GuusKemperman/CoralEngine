#include "Precomp.h"
#include "Assets/Core/AssetInternal.h"

#include "Assets/Asset.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Meta/MetaTools.h"
#include "Meta/MetaType.h"

CE::Internal::AssetInternal::AssetInternal(AssetFileMetaData&& metaData, const std::optional<std::filesystem::path>& path) :
	mMetaData(std::move(metaData)),
	mFileOfOrigin(path)
{
}

void CE::Internal::AssetInternal::Load()
{
	mHasBeenLoadedSinceGarbageCollect = true;

	LOG(LogAssets, Verbose, "Loading {}", mMetaData.GetName());

	if (mAsset != nullptr)
	{
		LOG(LogAssets, Warning, "Attempting to load {} twice", mMetaData.GetName());
		return;
	}

	if (!mFileOfOrigin.has_value())
	{
		LOG(LogAssets, Error, "Attempted to load {}, but this asset was generated at runtime and should not have been unloaded to begin with.",
			mMetaData.GetName());
		return;
	}

	std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromFile(*mFileOfOrigin);

	if (!loadInfo.has_value())
	{
		LOG(LogAssets, Error, "Asset {} could not be loaded, the metadata failed to load.",
			mMetaData.GetName());
		return;
	}

	ASSERT(loadInfo.has_value());

	FuncResult constructResult = mMetaData.GetClass().Construct(*loadInfo);

	if (constructResult.HasError())
	{
		LOG(LogAssets, Error, "Asset of type {} could not be constructed. Does it have a constructor that takes a LoadInfo&, and was this constructor reflected in your reflect function? {}",
			mMetaData.GetClass().GetName(),
			constructResult.Error());
		return;
	}
	mAsset = MakeUnique<Asset>(std::move(constructResult.GetReturnValue()));
}

void CE::Internal::AssetInternal::UnLoad()
{
	if (mAsset != nullptr)
	{
		LOG(LogAssets, Verbose, "Unloading {}", mMetaData.GetName());
		mAsset.reset();
	}
	else
	{
		LOG(LogAssets, Verbose, "Asset {} is already unloaded", mMetaData.GetName());
	}
}
