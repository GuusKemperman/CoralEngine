#pragma once
#include "Core/EngineSubsystem.h"

#include "Assets/Asset.h"
#include "Assets/Core/AssetFileMetaData.h"
#include "Assets/Core/AssetHandle.h"
#include "Assets/Core/AssetInternal.h"
#include "Meta/MetaManager.h"

namespace CE
{
	template<typename T>
	class WeakAssetHandle;

	class AssetManager final :
		public EngineSubsystem<AssetManager>
	{
		friend class EngineSubsystem<AssetManager>;
		void PostConstruct() override;

		~AssetManager();

	public:

		/*
		Get a loaded in asset by name.

		Returns:
			If the asset does not exist, this will return nullptr.
			If the asset is of a different class, this will return nullptr.
			Otherwise returns the shared_ptr to the asset.

		Example:
			AssetHandle<Prefab> player = AssetManager::Get().TryGetAsset<Prefab>("Player"_Name);
			AssetHandle<StaticMesh> mesh = AssetManager::Get().TryGetAsset<StaticMesh>(Name{ meshNameAsString });

			if (player != nullptr && mesh != nullptr) DoThing(player, mesh)
		*/
		template<typename T = Asset>
		AssetHandle<T> TryGetAsset(Name key);

		/*
		Get an asset which doesn't necessarily need to be loaded in. (See WeakAssetHandle).
		
		If you are unsure wether to call TryGetAsset or TryGetWeakAsset, 
		nine times out of ten you would use TryGetAsset. In some rare cases 
		where you only need some of the metadata, (e.g., the asset's class, 
		version or file of origin), this function can save on a lot of unnecesary 
		loading.
		
		Returns:
			If the asset does not exist, this will return nullopt.
			If the asset is of a different class, this will return nullopt.
			Otherwise returns the WeakAssetHandle.

		Example:
			WeakAssetHandle<Level> autoSave = AssetManager::Get().TryGetWeakAsset<Level>("AutoSave"_Name);

			if (autoSave != nullptr // We can test to see if an asset exists without having to load it in.
				|| autoSave.GetMetaData().GetAssetVersion() < 42) // We have access to some metadata without having to load it in.
			{
				LOG(LogGame, Warning, "Autosave does not exist or is out of date");
				return;
			}

			if (ImGui::Button("Revert to autosave?")
			{
				// Only if the user presses the button do we decide to load the level in.
				AssetHandle<Level> loadedLevel{ autoSave };
				DoThing(loadedLevel);
			}
		*/
		template<typename T = Asset>
		WeakAssetHandle<T> TryGetWeakAsset(Name key);

		/*
		If you want to save on memory, it is recommended to call this every frame
		or at an interval, at the cost that this will lead to assets 
		being loaded/unloaded more frequently.

		Assets generated at runtime are not unloaded automatically, because
		they cannot be loaded back in by the asset manager.
		*/
		void UnloadAllUnusedAssets();

		template<typename AssetType>
		class EachAssetIt
		{
		public:
			using value_type = WeakAssetHandle<AssetType>;
			using ContainerType = std::forward_list<Internal::AssetInternal>;
			using UnderlyingIt = ContainerType::iterator;

			EachAssetIt(UnderlyingIt&& it, ContainerType& container);

			decltype(auto) operator*() const;

			decltype(auto) operator->() const;

			EachAssetIt& operator++();
			EachAssetIt operator++(int);

			constexpr bool operator==(const EachAssetIt& b) const;
			constexpr bool operator!=(const EachAssetIt& b) const;

		private:
			void IncrementUntilTypeMatches();

			UnderlyingIt mIt;
			std::reference_wrapper<ContainerType> mContainer;
		};

		/*
		Returns all the assets of the given type
		*/
		template<typename T = Asset>
		IterableRange<EachAssetIt<T>> GetAllAssets();

		/*
		Rename the asset

		Will fail if there is already an asset with this name.

		Returns true on success.
		*/
		void RenameAsset(WeakAssetHandle<> asset, std::string_view newName);

		/*
		Will delete the file the asset originated from.
		The asset can then no longer be found through the asset manager.
		*/
		void DeleteAsset(WeakAssetHandle<>&& asset);

		/*
		Move the asset file.

		Will fail if there is already an asset at the specified location, or if the
		asset was generated at runtime.

		Returns true on success.
		*/
		bool MoveAsset(WeakAssetHandle<> asset, const std::filesystem::path& toFile);

		/*
		Duplicates the asset.

		Returns the duplicated asset on success.
		*/
		WeakAssetHandle<> Duplicate(WeakAssetHandle<> asset, const std::filesystem::path& copyPath);

		/*
		Will load the asset from the specified path.
		*/
		WeakAssetHandle<> NewAsset(const MetaType& assetClass, const std::filesystem::path& path);

		/*
		Add an asset generated at runtime to the asset manager.

		Will fail if there is already an asset with this name.

		The asset is not automatically offloaded when it is
		unreferenced. DeleteAsset must be explicitly called
		if you want to remove the asset.
		*/
		template<typename T>
		AssetHandle<T> AddAsset(T&& generatedAsset);

		/*
		Will load the asset from the specified path.
		*/
		WeakAssetHandle<> OpenAsset(const std::filesystem::path& path);

		static inline constexpr std::string_view sAssetExtension = ".asset";
		static inline constexpr std::string_view sRenameExtension = ".rename";

	private:
		// Made a friend, as the AssetManager is the only one
		// allowed to create weak assets. We have a ToWeakAsset function
		// that EachAssetT is allowed to use, without exposing the constructor
		// of WeakAssetHandle to our users.
		template<typename T>
		friend class EachAssetT;

		std::forward_list<Internal::AssetInternal> mAssets{};
		std::unordered_map<Name::HashType, std::reference_wrapper<Internal::AssetInternal>> mLookUp{};

		Internal::AssetInternal* TryGetAssetInternal(Name key, TypeId typeId);

		Internal::AssetInternal* TryConstruct(const std::filesystem::path& path);
		Internal::AssetInternal* TryConstruct(const std::optional<std::filesystem::path>& path, AssetFileMetaData metaData);
	};

	template <typename AssetType>
	constexpr bool AssetManager::EachAssetIt<AssetType>::operator==(const EachAssetIt& b) const
	{
		return mIt == b.mIt;
	}

	template <typename AssetType>
	constexpr bool AssetManager::EachAssetIt<AssetType>::operator!=(const EachAssetIt& b) const
	{
		return mIt != b.mIt;
	}

	template<typename T>
	AssetHandle<T> AssetManager::TryGetAsset(const Name key)
	{
		return { TryGetAssetInternal(key, MakeTypeId<T>()) };
	}

	template <typename T>
	WeakAssetHandle<T> AssetManager::TryGetWeakAsset(Name key)
	{
		return { TryGetAssetInternal(key, MakeTypeId<T>()) };
	}

	template <typename AssetType>
	AssetManager::EachAssetIt<AssetType>::EachAssetIt(UnderlyingIt&& it, ContainerType& container) :
		mIt(std::move(it)),
		mContainer(container)
	{
		// Not really a traditional iterator i suppose,
		// but good enough for our purposes.
		if (mIt == mContainer.get().begin()
			&& !mIt->mMetaData.GetClass().IsDerivedFrom<AssetType>())
		{
			IncrementUntilTypeMatches();
		}
	}

	template <typename AssetType>
	decltype(auto) AssetManager::EachAssetIt<AssetType>::operator*() const
	{
		return WeakAssetHandle<AssetType>{ &*mIt };
	}

	template <typename AssetType>
	decltype(auto) AssetManager::EachAssetIt<AssetType>::operator->() const
	{
		return **this;
	}

	template <typename AssetType>
	AssetManager::EachAssetIt<AssetType>& AssetManager::EachAssetIt<AssetType>::operator++()
	{
		IncrementUntilTypeMatches();
		return *this;
	}

	template <typename AssetType>
	AssetManager::EachAssetIt<AssetType> AssetManager::EachAssetIt<AssetType>::operator++(int)
	{
		EachAssetIt tmp = *this; ++(*this); return tmp;
	}

	template <typename AssetType>
	void AssetManager::EachAssetIt<AssetType>::IncrementUntilTypeMatches()
	{
		if constexpr (std::is_same_v<Asset, AssetType>)
		{
			++mIt;
		}
		else
		{
			do
			{
				++mIt;
			} while (mIt != mContainer.get().end()
				&& !mIt->mMetaData.GetClass().IsDerivedFrom<AssetType>());
		}
	}

	template <typename T>
	IterableRange<AssetManager::EachAssetIt<T>> AssetManager::GetAllAssets()
	{
		return { { mAssets.begin(), mAssets }, { mAssets.end(), mAssets } };
	}

	template <typename T>
	AssetHandle<T> AssetManager::AddAsset(T&& generatedAsset)
	{
		Internal::AssetInternal* internalAsset = TryConstruct(std::nullopt, AssetFileMetaData{ generatedAsset.GetName(), MetaManager::Get().GetType<T>() });

		if (internalAsset != nullptr)
		{
			internalAsset->mAsset = {
				[&generatedAsset]
				{
					return MakeUniqueInPlace<T, Asset>(std::move(generatedAsset));
				}
			};
			internalAsset->mAsset.GetThread().Join();
		}

		return { internalAsset };
	}
}
