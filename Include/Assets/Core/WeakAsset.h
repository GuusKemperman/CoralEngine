#pragma once
#include "AssetInternal.h"
#include "Assets/Asset.h"

namespace CE
{
	/*
	An asset that may or may not be loaded in, but does offer
	access to some metadata that is always available.

	WeakAsset is based on std::weak_ptr; WeakAssets do not
	influence the reference count.
	*/
	template<typename T = Asset>
	class WeakAsset
	{
		friend class AssetManager;
		WeakAsset(Internal::AssetInternal& assetInternal) : mAssetInternal(assetInternal) {};

	public:
		WeakAsset(const WeakAsset& other) = default;
		WeakAsset(WeakAsset&& other) noexcept = default;

		~WeakAsset() = default;

		WeakAsset& operator=(const WeakAsset&) = default;
		WeakAsset& operator=(WeakAsset&&) = default;

		const std::string& GetName() const { return mAssetInternal.get().mMetaData.GetName(); }

		const MetaType& GetAssetClass() const { return mAssetInternal.get().mMetaData.GetClass(); }

		bool IsLoaded() const { return mAssetInternal.get().mAsset != nullptr; }

		size_t NumOfReferences() const { return mAssetInternal.get().mAsset.use_count(); }

		uint32 GetAssetVersion() const { return mAssetInternal.get().mMetaData.GetAssetVersion(); }

		uint32 GetMetaDataVersion() const { return mAssetInternal.get().mMetaData.GetMetaDataVersion(); }

		std::shared_ptr<const T> MakeShared() const
		{
			if (!IsLoaded())
			{
				AssetManager::Get().Load(mAssetInternal);
				ASSERT(IsLoaded());
			}

			return std::static_pointer_cast<const T>(mAssetInternal.get().mAsset);
		}

		// May return either a .asset file. Will be nullopt if this was a generated asset.
		std::optional<std::filesystem::path> GetFileOfOrigin() const { return mAssetInternal.get().mFileOfOrigin; }

		std::optional<std::filesystem::path> GetImportedFromFile() const
		{
			const std::optional<AssetFileMetaData::ImporterInfo>& importerInfo = mAssetInternal.get().mMetaData.GetImporterInfo();
			return importerInfo.has_value() ? importerInfo->mImportedFile : std::optional<std::filesystem::path>{};
		}

		std::optional<uint32> GetImporterVersion() const
		{
			const std::optional<AssetFileMetaData::ImporterInfo>& importerInfo = mAssetInternal.get().mMetaData.GetImporterInfo();
			return importerInfo.has_value() ? importerInfo->mImporterVersion : std::optional<uint32>{};
		}

		std::optional<AssetFileMetaData::ImporterInfo> GetImporterInfo() const { return mAssetInternal.get().mMetaData.GetImporterInfo(); }

	private:
		std::reference_wrapper<Internal::AssetInternal> mAssetInternal;

		template<typename To, typename From>
		friend WeakAsset<To> WeakAssetStaticCast(WeakAsset<From>);
	};

	// TODO write dynamic cast as well
	template<typename To, typename From>
	WeakAsset<To> WeakAssetStaticCast(WeakAsset<From> other)
	{
		return WeakAsset<To>{ other.mAssetInternal };
	}
}
