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
		WeakAsset(Internal::AssetInternal& assetInternal);

	public:
		WeakAsset(const WeakAsset& other);
		WeakAsset(WeakAsset&& other) noexcept;

		~WeakAsset();

		WeakAsset& operator=(const WeakAsset&);
		WeakAsset& operator=(WeakAsset&&) noexcept;

		const AssetFileMetaData& GetMetaData() const;

		bool IsLoaded() const;

		size_t NumOfReferences() const;

		std::shared_ptr<const T> MakeShared() const;

		const std::optional<std::filesystem::path>& GetFileOfOrigin() const;

	private:
		std::reference_wrapper<Internal::AssetInternal> mAssetInternal;

		template<typename To, typename From>
		friend WeakAsset<To> WeakAssetStaticCast(WeakAsset<From>);
	};

	template <typename T>
	WeakAsset<T>::WeakAsset(Internal::AssetInternal& assetInternal): mAssetInternal(assetInternal)
	{}

	template <typename T>
	WeakAsset<T>::WeakAsset(const WeakAsset& other) = default;

	template <typename T>
	WeakAsset<T>::WeakAsset(WeakAsset&& other) noexcept = default;

	template <typename T>
	WeakAsset<T>::~WeakAsset() = default;

	template <typename T>
	WeakAsset<T>& WeakAsset<T>::operator=(const WeakAsset&) = default;

	template <typename T>
	WeakAsset<T>& WeakAsset<T>::operator=(WeakAsset&&) noexcept = default;

	template <typename T>
	const AssetFileMetaData& WeakAsset<T>::GetMetaData() const
	{
		return mAssetInternal.get().mMetaData;
	}

	template <typename T>
	bool WeakAsset<T>::IsLoaded() const
	{
		return mAssetInternal.get().mAsset != nullptr;
	}

	template <typename T>
	size_t WeakAsset<T>::NumOfReferences() const
	{
		return mAssetInternal.get().mAsset.use_count();
	}

	template <typename T>
	std::shared_ptr<const T> WeakAsset<T>::MakeShared() const
	{
		if (!IsLoaded())
		{
			mAssetInternal.get().Load();
			ASSERT(IsLoaded());
		}

		return std::static_pointer_cast<const T>(mAssetInternal.get().mAsset);
	}

	template <typename T>
	const std::optional<std::filesystem::path>& WeakAsset<T>::GetFileOfOrigin() const
	{
		return mAssetInternal.get().mFileOfOrigin;
	}

	// TODO write dynamic cast as well
	template<typename To, typename From>
	WeakAsset<To> WeakAssetStaticCast(WeakAsset<From> other)
	{
		return WeakAsset<To>{ other.mAssetInternal };
	}
}
