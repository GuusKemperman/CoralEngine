#pragma once
#include "AssetInternal.h"
#include "AssetInternal.h"
#include "Assets/Asset.h"

namespace CE
{
	class AssetHandleBase
	{
	public:
		AssetHandleBase();
		AssetHandleBase(nullptr_t);

		AssetHandleBase(AssetHandleBase&& other) noexcept;

		AssetHandleBase& operator=(AssetHandleBase&& other) noexcept;

		bool operator==(nullptr_t) const;
		bool operator!=(nullptr_t) const;
		
		operator bool() const;

		const AssetFileMetaData& GetMetaData() const;

		bool IsLoaded() const;

		const std::optional<std::filesystem::path>& GetFileOfOrigin() const;

		uint32 GetNumberOfStrongReferences() const;
		uint32 GetNumberOfSoftReferences() const;

	protected:
		void AssureNotNull() const;

		Internal::AssetInternal* mAssetInternal{};
	};

	template<Internal::AssetInternal::RefCountType IndexOfCounter>
	class AssetHandleRefCounter :
		public AssetHandleBase
	{
	public:
		AssetHandleRefCounter();

		AssetHandleRefCounter(nullptr_t);

		AssetHandleRefCounter(Internal::AssetInternal* assetInternal);

		AssetHandleRefCounter(AssetHandleRefCounter&& other) noexcept;

		AssetHandleRefCounter(const AssetHandleRefCounter& other);

		AssetHandleRefCounter& operator=(AssetHandleRefCounter&& other) noexcept;

		AssetHandleRefCounter& operator=(const AssetHandleRefCounter& other);

		AssetHandleRefCounter& operator=(nullptr_t);

		~AssetHandleRefCounter();

	private:
		void IncreaseRef();

		void DecreaseRef();
	};

	template<typename T = Asset>
	class AssetHandle final :
		public AssetHandleRefCounter<Internal::AssetInternal::RefCountType::Strong>
	{
	public:
		using AssetHandleRefCounter::AssetHandleRefCounter;

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		AssetHandle(AssetHandle<O>&& other) noexcept;

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		AssetHandle(const AssetHandle<O>& other);

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		AssetHandle& operator=(AssetHandle<O>&& other) noexcept;

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		AssetHandle& operator=(const AssetHandle<O>& other);

		const T& operator*() const;
		const T* operator->() const;
	};

	template<typename T = Asset>
	class WeakAssetHandle final :
		public AssetHandleRefCounter<Internal::AssetInternal::RefCountType::Strong>
	{
	public:
		using AssetHandleRefCounter::AssetHandleRefCounter;

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		WeakAssetHandle(WeakAssetHandle<O>&& other) noexcept;

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		WeakAssetHandle(const WeakAssetHandle<O>& other);

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		WeakAssetHandle& operator=(WeakAssetHandle<O>&& other) noexcept;

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		WeakAssetHandle& operator=(const WeakAssetHandle<O>& other);

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		explicit operator AssetHandle<O>() const;
	};

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter() = default;

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter(nullptr_t)
	{}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter(Internal::AssetInternal* assetInternal)
	{
		mAssetInternal = assetInternal;
		IncreaseRef();
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter(AssetHandleRefCounter&& other) noexcept :
		AssetHandleBase(std::move(other))
	{}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter(const AssetHandleRefCounter& other)
	{
		mAssetInternal = other.mAssetInternal;
		IncreaseRef();
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>& AssetHandleRefCounter<IndexOfCounter>::operator=(
		AssetHandleRefCounter&& other) noexcept
	{
		return static_cast<AssetHandleRefCounter&>(AssetHandleBase::operator=(std::move(other)));
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>& AssetHandleRefCounter<IndexOfCounter>::operator=(
		const AssetHandleRefCounter& other)
	{
		DecreaseRef();
		mAssetInternal = other.mAssetInternal;
		IncreaseRef();
		return *this;
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>& AssetHandleRefCounter<IndexOfCounter>::operator=(nullptr_t)
	{
		DecreaseRef();
		return *this;
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::~AssetHandleRefCounter()
	{
		DecreaseRef();
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	void AssetHandleRefCounter<IndexOfCounter>::IncreaseRef()
	{
		if (mAssetInternal != nullptr)
		{
			++mAssetInternal->mRefCounters[static_cast<int>(IndexOfCounter)];
		}
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	void AssetHandleRefCounter<IndexOfCounter>::DecreaseRef()
	{
		if (mAssetInternal != nullptr)
		{
			--mAssetInternal->mRefCounters[static_cast<int>(IndexOfCounter)];
		}
	}

	template <typename T>
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	AssetHandle<T>::AssetHandle(AssetHandle<O>&& other) noexcept :
		AssetHandleRefCounter(std::move(other))
	{
	}

	template <typename T>
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	AssetHandle<T>::AssetHandle(const AssetHandle<O>& other) :
		AssetHandleRefCounter(other)
	{
	}

	template <typename T>
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	AssetHandle<T>& AssetHandle<T>::operator=(AssetHandle<O>&& other) noexcept
	{
		return static_cast<AssetHandle&>(AssetHandleRefCounter::operator=(std::move(other)));
	}

	template <typename T>
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	AssetHandle<T>& AssetHandle<T>::operator=(const AssetHandle<O>& other)
	{
		return static_cast<AssetHandle&>(AssetHandleRefCounter::operator=(other));
	}

	template <typename T>
	const T& AssetHandle<T>::operator*() const
	{
		ASSERT(IsLoaded());
		return *mAssetInternal->mAsset;
	}

	template <typename T>
	const T* AssetHandle<T>::operator->() const
	{
		ASSERT(IsLoaded());
		return *mAssetInternal->mAsset;
	}

	template <typename T>
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	WeakAssetHandle<T>::WeakAssetHandle(WeakAssetHandle<O>&& other) noexcept :
		AssetHandleRefCounter(std::move(other))
	{
	}

	template <typename T>
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	WeakAssetHandle<T>::WeakAssetHandle(const WeakAssetHandle<O>& other) :
		AssetHandleRefCounter(other)
	{
	}

	template <typename T>
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	WeakAssetHandle<T>& WeakAssetHandle<T>::operator=(WeakAssetHandle<O>&& other) noexcept
	{
		return static_cast<WeakAssetHandle&>(AssetHandleRefCounter::operator=(std::move(other)));
	}

	template <typename T>
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	WeakAssetHandle<T>& WeakAssetHandle<T>::operator=(const WeakAssetHandle<O>& other)
	{
		return static_cast<WeakAssetHandle&>(AssetHandleRefCounter::operator=(std::move(other)));
	}

	template <typename T>
	template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
	WeakAssetHandle<T>::operator AssetHandle<O>() const
	{
		return AssetHandle<T>{ mAssetInternal };
	}
}
