#include "Precomp.h"
#include "Assets/Core/AssetInternal.h"

#include "Assets/Asset.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Meta/MetaTools.h"
#include "Meta/MetaType.h"

CE::Internal::AssetInternal::AssetInternal(AssetMetaData metaData, std::optional<std::filesystem::path> path) :
	mMetaData(std::move(metaData)),
	mFileOfOrigin(std::move(path))
{
}

CE::Internal::AssetInternal::AssetInternal(std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> asset) :
	mAsset(std::move(asset)),
	mMetaData(mAsset->GetName(), [this]() -> decltype(auto)
	{
		const MetaType* type = MetaManager::Get().TryGetType(mAsset->GetTypeId());
		ASSERT(type != nullptr);
		return *type;
	}())
{
}

CE::Asset* CE::Internal::AssetInternal::TryGetLoadedAsset()
{
	std::unique_lock lock{ mAccessMutex };

	if (mAsset != nullptr)
	{
		return mAsset.get();
	}

	if (!mFileOfOrigin.has_value())
	{
		LOG(LogAssets, Error, "Attempted to load {}, but this asset was generated at runtime and should not have been unloaded to begin with.",
			mMetaData.GetName());
		return nullptr;
	}

	std::optional<AssetLoadInfo> loadInfo = AssetLoadInfo::LoadFromFile(*mFileOfOrigin);

	if (!loadInfo.has_value())
	{
		LOG(LogAssets, Error, "Asset {} could not be loaded, the metadata failed to load.",
			mMetaData.GetName());
		return nullptr;
	}

	ASSERT(loadInfo.has_value());

	FuncResult constructResult = mMetaData.GetClass().Construct(*loadInfo);

	if (constructResult.HasError())
	{
		LOG(LogAssets, Error, "Asset of type {} could not be constructed. Does it have a constructor that takes a LoadInfo&, and was this constructor reflected in your reflect function? {}",
			mMetaData.GetClass().GetName(),
			constructResult.Error());
		return nullptr;
	}
	mAsset = MakeUnique<Asset>(std::move(constructResult.GetReturnValue()));
	return mAsset.get();
}
