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

CE::Asset& CE::Internal::AssetInternal::Get()
{
	if (mAsset.IsReady())
	{
		return *mAsset.Get();
	}

	mLoadUnloadMutex.lock();

	if (mAsset.IsReady())
	{
		mLoadUnloadMutex.unlock();
		return *mAsset.Get();
	}

	if (!mAsset.GetThread().WasLaunched())
	{
		mAsset =
		{
			[this]() -> std::unique_ptr<Asset, InPlaceDeleter<Asset, true>>
			{
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

				LOG(LogAssets, Verbose, "Loading {}", mMetaData.GetName());

				ASSERT(loadInfo.has_value());

				FuncResult constructResult = mMetaData.GetClass().Construct(*loadInfo);

				if (constructResult.HasError())
				{
					LOG(LogAssets, Error, "Asset of type {} could not be constructed. Does it have a constructor that takes a LoadInfo&, and was this constructor reflected in your reflect function? {}",
						mMetaData.GetClass().GetName(),
						constructResult.Error());

					return nullptr;
				}
				return MakeUnique<Asset>(std::move(constructResult.GetReturnValue()));
			}
		};
	}
	mAsset.GetThread().Join();
	ASSERT(mAsset.IsReady());
	mLoadUnloadMutex.unlock();
	return *mAsset.Get();
}

void CE::Internal::AssetInternal::StartLoadingIfNotStarted()
{
	(void)TryGet();
}

void CE::Internal::AssetInternal::UnloadIfLoaded()
{
	mLoadUnloadMutex.lock();

	if (mAsset.GetThread().WasLaunched())
	{
		mAsset.GetThread().CancelOrJoin();
	}
	mAsset = {};

	mLoadUnloadMutex.unlock();
}

bool CE::Internal::AssetInternal::IsLoaded() const
{
	return mAsset.IsReady();
}

CE::Asset* CE::Internal::AssetInternal::TryGet()
{
	if (mAsset.IsReady())
	{
		return mAsset.Get().get();
	}

	if (!mAsset.GetThread().WasLaunched())
	{
		mLoadUnloadMutex.lock();

		if (!mAsset.GetThread().WasLaunched())
		{
			mAsset =
			{
				[this]() -> std::unique_ptr<Asset, InPlaceDeleter<Asset, true>>
				{
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

					LOG(LogAssets, Verbose, "Loading {}", mMetaData.GetName());

					ASSERT(loadInfo.has_value());

					FuncResult constructResult = mMetaData.GetClass().Construct(*loadInfo);

					if (constructResult.HasError())
					{
						LOG(LogAssets, Error, "Asset of type {} could not be constructed. Does it have a constructor that takes a LoadInfo&, and was this constructor reflected in your reflect function? {}",
							mMetaData.GetClass().GetName(),
							constructResult.Error());

						return nullptr;
					}
					return MakeUnique<Asset>(std::move(constructResult.GetReturnValue()));
				}
			};
		}

		mLoadUnloadMutex.unlock();
	}

	if (mAsset.IsReady())
	{
		return mAsset.Get().get();
	}
	return nullptr;
}
