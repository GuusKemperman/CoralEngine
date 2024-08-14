#pragma once
#include "AssetInternal.h"
#include "Assets/Asset.h"
#include "Meta/MetaReflect.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"

namespace CE
{
	class AssetHandleBase
	{
	public:
		AssetHandleBase();
		AssetHandleBase(nullptr_t);

		AssetHandleBase(std::shared_ptr<Internal::AssetInternal> assetInternal);

		AssetHandleBase(AssetHandleBase&& other) noexcept;

		AssetHandleBase& operator=(AssetHandleBase&& other) noexcept;

		bool operator==(nullptr_t) const;
		bool operator!=(nullptr_t) const;

		bool operator==(const AssetHandleBase& other) const;
		bool operator!=(const AssetHandleBase& other) const;

		operator bool() const;

		const AssetMetaData& GetMetaData() const;

		bool IsLoaded() const;

		const std::optional<std::filesystem::path>& GetFileOfOrigin() const;

		/**
		 * \brief If this asset was renamed, this will return the names that this asset previously went by.
		 */
		std::vector<std::string> GetOldNames() const;

		uint32 GetNumberOfStrongReferences() const;
		uint32 GetNumberOfSoftReferences() const;

	protected:
		const Asset* GetAssetBase() const;

		void AssureNotNull() const;

		bool IsA(TypeId type) const;

		std::shared_ptr<Internal::AssetInternal> mAssetInternal{};
	};

	template<Internal::AssetInternal::RefCountType IndexOfCounter>
	class AssetHandleRefCounter :
		public AssetHandleBase
	{
	public:
		AssetHandleRefCounter();

		AssetHandleRefCounter(nullptr_t);

		AssetHandleRefCounter(std::shared_ptr<Internal::AssetInternal> assetInternal);

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
	class AssetHandle;

	template<typename T = Asset>
	class WeakAssetHandle;

	/**
	 * \brief Create an assethandle whose lifetime is not managed by the asset manager.
	 *
	 * The asset is destroyed when there are no AssetHandles or WeakAssetHandles that
	 * reference the asset.
	 *
	 * \param args The arguments used to construct an instance of the asset
	 */
	template<typename T, typename... Args>
	AssetHandle<T> MakeAssetHandle(Args&&... args);

	template<typename T, typename O, std::enable_if_t<std::is_convertible_v<T*, O*>, bool> = true>
	AssetHandle<T> StaticAssetHandleCast(const AssetHandle<O>& other);

	template<typename T, typename O, std::enable_if_t<std::is_convertible_v<T*, O*>, bool> = true>
	AssetHandle<T> DynamicAssetHandleCast(const AssetHandle<O>& other);

	template<typename T, typename O, std::enable_if_t<std::is_convertible_v<T*, O*>, bool> = true>
	WeakAssetHandle<T> StaticAssetHandleCast(const WeakAssetHandle<O>& other);

	template<typename T, typename O, std::enable_if_t<std::is_convertible_v<T*, O*>, bool> = true>
	WeakAssetHandle<T> DynamicAssetHandleCast(const WeakAssetHandle<O>& other);

	template<typename T>
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

		template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool> = true>
		operator WeakAssetHandle<O>() const;

		const T& operator*() const;
		const T* operator->() const;

		const T* Get() const;

	private:
		template<typename U, typename O, std::enable_if_t<std::is_convertible_v<U*, O*>, bool>>
		friend AssetHandle<U> StaticAssetHandleCast(const AssetHandle<O>& other);

		template<typename U, typename O, std::enable_if_t<std::is_convertible_v<U*, O*>, bool>>
		friend AssetHandle<U> DynamicAssetHandleCast(const AssetHandle<O>& other);

		friend ReflectAccess;
		static MetaType Reflect();
	};

	template<typename T>
	class WeakAssetHandle final :
		public AssetHandleRefCounter<Internal::AssetInternal::RefCountType::Weak>
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
		operator AssetHandle<O>() const;

	private:
		template<typename U, typename O, std::enable_if_t<std::is_convertible_v<U*, O*>, bool>>
		friend WeakAssetHandle<U> StaticAssetHandleCast(const WeakAssetHandle<O>& other);

		template<typename U, typename O, std::enable_if_t<std::is_convertible_v<U*, O*>, bool>>
		friend WeakAssetHandle<U> DynamicAssetHandleCast(const WeakAssetHandle<O>& other);

		friend ReflectAccess;
		static MetaType Reflect();
	};

	template<typename T>
	AssetHandle(WeakAssetHandle<T>) -> AssetHandle<T>;

	template<typename T>
	WeakAssetHandle(AssetHandle<T>) -> WeakAssetHandle<T>;

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter() = default;

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter(nullptr_t)
	{}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter(std::shared_ptr<Internal::AssetInternal> assetInternal)
	{
		mAssetInternal = std::move(assetInternal);
		IncreaseRef();
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter(AssetHandleRefCounter&& other) noexcept :
		AssetHandleBase(std::move(other))
	{
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>::AssetHandleRefCounter(const AssetHandleRefCounter& other)
	{
		DecreaseRef(); // Our ref is overwritten
		mAssetInternal = other.mAssetInternal;
		IncreaseRef(); // We made a copy, so increment
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>& AssetHandleRefCounter<IndexOfCounter>::operator=(AssetHandleRefCounter&& other) noexcept
	{
		if (&other == this)
		{
			return *this;
		}

		DecreaseRef(); // Our ref is overwritten
		mAssetInternal = std::move(other.mAssetInternal);
		// Ref count is not incremented; we stole it from other

		return *this;
	}

	template <Internal::AssetInternal::RefCountType IndexOfCounter>
	AssetHandleRefCounter<IndexOfCounter>& AssetHandleRefCounter<IndexOfCounter>::operator=(const AssetHandleRefCounter& other)
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

	template <typename T, typename ... Args>
	AssetHandle<T> MakeAssetHandle(Args&&... args)
	{
		std::unique_ptr<Asset, InPlaceDeleter<Asset, true>> asset = MakeUniqueInPlace<T, Asset>(std::forward<Args>(args)...);
		std::shared_ptr<Internal::AssetInternal> assetInternal = std::make_shared<Internal::AssetInternal>(std::move(asset));
		return { std::move(assetInternal) };
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
	template <typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	AssetHandle<T>::operator WeakAssetHandle<O>() const
	{
		return { mAssetInternal };
	}

	template <typename T>
	const T& AssetHandle<T>::operator*() const
	{
		AssureNotNull();
		return *Get();
	}

	template <typename T>
	const T* AssetHandle<T>::operator->() const
	{
		return Get();
	}

	template <typename T>
	const T* AssetHandle<T>::Get() const
	{
		// We do a reinterpret_cast,
		// otherwise if we use a static_cast
		// the user is forced to include the
		// entire header of where the asset is
		// located.
		return reinterpret_cast<const T*>(GetAssetBase());
	}

	template <typename T, typename O, std::enable_if_t<std::is_convertible_v<T*, O*>, bool>>
	AssetHandle<T> StaticAssetHandleCast(const AssetHandle<O>& other)
	{
		return { other.mAssetInternal };
	}

	template <typename T, typename O, std::enable_if_t<std::is_convertible_v<T*, O*>, bool>>
	AssetHandle<T> DynamicAssetHandleCast(const AssetHandle<O>& other)
	{
		if (other.IsA(MakeTypeId<T>()))
		{
			return StaticAssetHandleCast<T>(other);
		}
		return nullptr;
	}

	template <typename T, typename O, std::enable_if_t<std::is_convertible_v<T*, O*>, bool>>
	WeakAssetHandle<T> StaticAssetHandleCast(const WeakAssetHandle<O>& other)
	{
		return { other.mAssetInternal };
	}

	template <typename T, typename O, std::enable_if_t<std::is_convertible_v<T*, O*>, bool>>
	WeakAssetHandle<T> DynamicAssetHandleCast(const WeakAssetHandle<O>& other)
	{
		if (other.IsA(MakeTypeId<T>()))
		{
			return StaticAssetHandleCast<T>(other);
		}
		return nullptr;
	}

	template <typename U>
	MetaType AssetHandle<U>::Reflect()
	{
		const MetaType& basedOnType = MetaManager::Get().GetType<U>();
		MetaType refType{ MetaType::T<AssetHandle<U>>{}, Format("{} Ref", basedOnType.GetName()) };
		refType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

		ReflectFieldType<AssetHandle<U>>(refType);

		refType.AddFunc(
			[](const AssetHandle<U>& value, nullptr_t)
			{
				return value == nullptr;
			}, OperatorType::equal);

		return refType;
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
	template<typename O, std::enable_if_t<std::is_convertible_v<O*, T*>, bool>>
	WeakAssetHandle<T>::operator AssetHandle<O>() const
	{
		return AssetHandle<T>{ mAssetInternal };
	}

	template <typename U>
	MetaType WeakAssetHandle<U>::Reflect()
	{
		const MetaType& basedOnType = MetaManager::Get().GetType<U>();
		MetaType refType{ MetaType::T<WeakAssetHandle<U>>{}, Format("{} Weak Ref", basedOnType.GetName()) };

		ReflectFieldType<WeakAssetHandle<U>>(refType);

		refType.AddFunc(
			[](const WeakAssetHandle<U>& value, nullptr_t)
			{
				return value == nullptr;
			}, OperatorType::equal);

		return refType;
	}
}

template<typename T>
struct std::hash<CE::AssetHandle<T>>
{
	std::size_t operator()(const CE::AssetHandle<T>& asset) const
	{
		return std::hash<const T*>{}(asset.Get());
	}
};

namespace cereal
{
	class BinaryInputArchive;
	class BinaryOutputArchive;

	void save(BinaryOutputArchive& archive, const CE::AssetHandleBase& asset);

	void load(BinaryInputArchive& archive, CE::AssetHandle<>& asset);

	void load(BinaryInputArchive& archive, CE::WeakAssetHandle<>& asset);

	template<typename T>
	void load(BinaryInputArchive& archive, CE::AssetHandle<T>& asset)
	{
		CE::AssetHandle<> asBase = asset;
		load(archive, asBase);
		asset = CE::DynamicAssetHandleCast<T>(asBase);
	}

	template<typename T>
	void load(BinaryInputArchive& archive, CE::WeakAssetHandle<T>& asset)
	{
		CE::WeakAssetHandle<> asBase = asset;
		load(archive, asBase);
		asset = CE::DynamicAssetHandleCast<T>(asBase);
	}
}

#ifdef EDITOR

namespace CE::Internal
{
	void DisplayHandleWidget(AssetHandle<>& asset, const std::string& name, TypeId type);
	void DisplayHandleWidget(WeakAssetHandle<>& asset, const std::string& name, TypeId type);
}

namespace ImGui
{
	template<typename T>
	struct Auto_t<CE::AssetHandle<T>>
	{
		static void Auto(CE::AssetHandle<T>& asset, const std::string& name)
		{
			CE::AssetHandle<> base = asset;
			CE::Internal::DisplayHandleWidget(base, name, CE::MakeTypeId<T>());
			asset = CE::StaticAssetHandleCast<T>(base);
		}
		static constexpr bool sIsSpecialized = true;
	};

	template<typename T>
	struct Auto_t<CE::WeakAssetHandle<T>>
	{
		static void Auto(CE::WeakAssetHandle<T>& asset, const std::string& name)
		{
			CE::WeakAssetHandle<> base = asset;
			CE::Internal::DisplayHandleWidget(base, name, CE::MakeTypeId<T>());
			asset = CE::StaticAssetHandleCast<T>(base);
		}
		static constexpr bool sIsSpecialized = true;
	};
}
#endif // EDITOR