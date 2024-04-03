#pragma once
#include "Core/EngineSubsystem.h"

#include "Assets/Asset.h"
#include "Assets/Core/AssetFileMetaData.h"
#include "Assets/Core/AssetInternal.h"
#include "Assets/Core/WeakAsset.h"
#include "Meta/MetaTypeId.h"
#include "Meta/MetaManager.h"

namespace CE
{
	template<typename T>
	class WeakAsset;

	class AssetManager final :
		public EngineSubsystem<AssetManager>
	{
		friend class EngineSubsystem<AssetManager>;
		void PostConstruct() override;

	public:

		/*
		Get a loaded in asset by name.

		Returns:
			If the asset does not exist, this will return nullptr.
			If the asset is of a different class, this will return nullptr.
			Otherwise returns the shared_ptr to the asset.

		Example:
			std::shared_ptr<const Prefab> player = AssetManager::Get().TryGetAsset<Prefab>("Player"_Name);
			std::shared_ptr<const StaticMesh> mesh = AssetManager::Get().TryGetAsset<StaticMesh>(Name{ meshNameAsString });

			if (player != nullptr && mesh != nullptr) DoThing(player, mesh)
		*/
		template<typename T = Asset>
		std::shared_ptr<const T> TryGetAsset(Name key);

		/*
		Get an asset which doesn't necessarily need to be loaded in. (See WeakAsset).
		
		If you are unsure wether to call TryGetAsset or TryGetWeakAsset, 
		nine times out of ten you would use TryGetAsset. In some rare cases 
		where you only need some of the metadata, (e.g., the asset's class, 
		version or file of origin), this function can save on a lot of unnecesary 
		loading.
		
		Returns:
			If the asset does not exist, this will return nullopt.
			If the asset is of a different class, this will return nullopt.
			Otherwise returns the WeakAsset.

		Example:
			std::optional<WeakAsset<Level>> autoSave = AssetManager::Get().TryGetWeakAsset<Level>("AutoSave"_Name);

			if (!autoSave.has_value() // We can test to see if an asset exists without having to load it in.
				|| autoSave->GetAssetVersion() < ClassVersion_v<Level>) // We have access to some metadata without having to load it in.
			{
				LOG(LogGame, Warning, "Autosave does not exist or is out of date");
				return;
			}

			if (ImGui::Button("Revert to autosave?")
			{
				// Only if the user presses the button do we decide to load the level in.
				std::shared_ptr<const Level> loadedLevel = autoSave->MakeShared();
				DoThing(loadedLevel);
			}
		*/
		template<typename T = Asset>
		std::optional<WeakAsset<T>> TryGetWeakAsset(Name key);

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
			using value_type = WeakAsset<AssetType>;
			using ContainerType = std::unordered_map<Name::HashType, Internal::AssetInternal>;
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
		template<typename AssetType = Asset>
 		IterableRange<EachAssetIt<AssetType>> GetAllAssets();

		/*
		Rename the asset

		Will fail if there is already an asset with this name.

		Returns true on success.
		*/
		void RenameAsset(WeakAsset<Asset> asset, std::string_view newName);

		/*
		Will delete the file the asset originated from.
		The asset can then no longer be found through the asset manager.
		*/
		void DeleteAsset(WeakAsset<Asset>&& asset);

		/*
		Move the asset file.

		Will fail if there is already an asset at the specified location, or if the
		asset was generated at runtime.

		Returns true on success.
		*/
		bool MoveAsset(WeakAsset<Asset> asset, const std::filesystem::path& toFile);

		/*
		Duplicates the asset.

		Returns the duplicated asset on success.
		*/
		std::optional<WeakAsset<Asset>> Duplicate(WeakAsset<Asset> asset, const std::filesystem::path& copyPath);

		/*
		Will load the asset from the specified path.
		*/
		std::optional<WeakAsset<Asset>> NewAsset(const MetaType& assetClass, const std::filesystem::path& path);

		/*
		Add an asset generated at runtime to the asset manager.

		Will fail if there is already an asset with this name.

		The asset is not automatically offloaded when it is
		unreferenced. DeleteAsset must be explicitly called
		if you want to remove the asset.
		*/
		template<typename T>
		std::shared_ptr<const T> AddAsset(T&& generatedAsset);

		/*
		Will load the asset from the specified path.
		*/
		std::optional<WeakAsset<Asset>> OpenAsset(const std::filesystem::path& path);

		static inline constexpr std::string_view sAssetExtension = ".asset";

	private:
		template<typename T>
		friend class WeakAsset;

		// Made a friend, as the AssetManager is the only one
		// allowed to create weak assets. We have a ToWeakAsset function
		// that EachAssetT is allowed to use, without exposing the constructor
		// of WeakAsset to our users.
		template<typename T>
		friend class EachAssetT;

		template<typename T>
		static WeakAsset<T> ToWeakAsset(Internal::AssetInternal& internalAsset);

		std::unordered_map<Name::HashType, Internal::AssetInternal> mAssets{};

		void OpenDirectory(const std::filesystem::path& directory);



		Internal::AssetInternal* TryGetAssetInternal(Name key, TypeId typeId);
		Internal::AssetInternal* TryGetLoadedAssetInternal(Name key, TypeId typeId);

		void Load(Internal::AssetInternal& asset);
		void Unload(Internal::AssetInternal& asset);

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
	std::shared_ptr<const T> AssetManager::TryGetAsset(const Name key)
	{
		const auto* assetInternal = TryGetLoadedAssetInternal(key, MakeTypeId<T>());
		return assetInternal == nullptr ? nullptr : std::static_pointer_cast<const T>(assetInternal->mAsset);
	}

	template<typename T>
	std::optional<WeakAsset<T>> AssetManager::TryGetWeakAsset(const Name key)
	{
		Internal::AssetInternal* assetInternal = TryGetAssetInternal(key, MakeTypeId<T>());

		if (assetInternal == nullptr)
		{
			return std::nullopt;
		}

		return WeakAsset<T>{ *assetInternal };
	}

	template <typename AssetType>
	AssetManager::EachAssetIt<AssetType>::EachAssetIt(UnderlyingIt&& it, ContainerType& container):
		mIt(std::move(it)),
		mContainer(container)
	{
		// Not really a traditional iterator i suppose,
		// but good enough for our purposes.
		if (mIt == mContainer.get().begin()
			&& !mIt->second.mMetaData.GetClass().IsDerivedFrom<AssetType>())
		{
			IncrementUntilTypeMatches();
		}
	}

	template <typename AssetType>
	decltype(auto) AssetManager::EachAssetIt<AssetType>::operator*() const
	{
		return AssetManager::ToWeakAsset<AssetType>(mIt->second);
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
				&& !mIt->second.mMetaData.GetClass().IsDerivedFrom<AssetType>());
		}
	}

	template <typename AssetType>
	IterableRange<AssetManager::EachAssetIt<AssetType>> AssetManager::GetAllAssets()
	{
		return { { mAssets.begin(), mAssets }, { mAssets.end(), mAssets } };
	}

	template <typename T>
	std::shared_ptr<const T> AssetManager::AddAsset(T&& generatedAsset)
	{
		Internal::AssetInternal* const assetInternal = TryConstruct(std::nullopt, AssetFileMetaData{ generatedAsset.GetName(), MetaManager::Get().GetType<T>() });

		if (assetInternal == nullptr)
		{
			return nullptr;
		}

		std::shared_ptr<T> ptr = std::make_shared<T>(std::forward<T>(generatedAsset));
		assetInternal->mAsset = ptr;
		return ptr;
	}

	template <typename T>
	WeakAsset<T> AssetManager::ToWeakAsset(Internal::AssetInternal& internalAsset)
	{
		return { internalAsset };
	}
}
