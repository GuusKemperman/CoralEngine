#pragma once
#include "Core/EngineSubsystem.h"

#include "Assets/Asset.h"
#include "Assets/Core/AssetFileMetaData.h"
#include "Meta/MetaTypeId.h"
#include "Meta/MetaManager.h"
#include "Assets/Importers/Importer.h"
#include "Utilities/MemFunctions.h"

namespace Engine
{
	template<typename T>
	class WeakAsset;

	class AssetManager final :
		public EngineSubsystem<AssetManager>
	{
		friend class EngineSubsystem<AssetManager>;
		AssetManager();
		~AssetManager();

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
				|| autoSave->GetVersion() < ClassVersion_v<Level>) // We have access to some metadata without having to load it in.
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

		/*
		Returns all the assets
		*/
		std::vector<WeakAsset<Asset>> GetAllAssets();

#ifdef EDITOR
		/*
		Imports the specified path, for example ruins.gltf.
		The GLTFImporter then returns all the assets inside
		ruins.gltf, each of which is then stored in their own
		.asset file.

		Note that the assets may not immediately be available;
		importing could mean swapping out existing assets, and
		since it's saver to do this when all assets are unreferenced.
		this is done at the end of the frame.
		*/
		void Import(const std::filesystem::path& path);
#endif

		/*
		Move the asset file.

		Will fail if there is already an asset at the specified location, or if the
		asset was generated at runtime.

		Returns true on success.
		*/
		bool MoveAsset(WeakAsset<Asset> asset, const std::filesystem::path& toFile);

		/*
		Will load the asset from the specified path.
		*/
		std::optional<WeakAsset<Asset>> AddAsset(const std::filesystem::path& path);

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
		Will delete the file the asset originated from.
		The asset can then no longer be found through the asset manager.

		WARNING: All WeakAssets referencing this asset, including the one passed
		in as an argument, will be invalidated and become dangling.

		Existing Shared pointers will not be invalided.
		*/
		void DeleteAsset(WeakAsset<Asset>&& asset);

		static inline constexpr std::string_view sAssetExtension = ".asset";

	private:
		template<typename T>
		friend class WeakAsset;

		// A potentially unloaded asset.
		class AssetInternal
		{
		public:
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
		std::unordered_map<Name::HashType, AssetInternal> mAssets{};

		void OpenDirectory(const std::filesystem::path& directory);

		AssetInternal* TryGetAssetInternal(Name key, TypeId typeId);
		AssetInternal* TryGetLoadedAssetInternal(Name key, TypeId typeId);

		void Load(AssetInternal& asset);
		void Unload(AssetInternal& asset);

#ifdef EDITOR
		static bool WasImportedFrom(const AssetInternal& asset, const std::filesystem::path& file);

		void ImportInternal(const std::filesystem::path& path, bool refreshEngine);

		std::pair<TypeId, const Importer*> TryGetImporterForExtension(const std::filesystem::path& extension) const;
#endif // EDITOR

		AssetInternal* TryConstruct(const std::filesystem::path& path);
		AssetInternal* TryConstruct(const std::optional<std::filesystem::path>& path, AssetFileMetaData metaData);


		// Importers are created using the runtime reflection system,
		// which uses placement new for the constructing of objects.
		// Hence, the custom deleter
		std::vector<std::pair<TypeId, std::unique_ptr<Importer, InPlaceDeleter<Importer, true>>>> mImporters{};
	};

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
		WeakAsset(AssetManager::AssetInternal& assetInternal) : mAssetInternal(assetInternal) {};

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

		uint32 GetVersion() const { return mAssetInternal.get().mMetaData.GetVersion(); }

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

	private:
		std::reference_wrapper<AssetManager::AssetInternal> mAssetInternal;

		template<typename To, typename From>
		friend WeakAsset<To> WeakAssetStaticCast(WeakAsset<From>);
	};

	// TODO write dynamic cast as well
	template<typename To, typename From>
	WeakAsset<To> WeakAssetStaticCast(WeakAsset<From> other)
	{
		return WeakAsset<To>{ other.mAssetInternal };
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
		AssetInternal* assetInternal = TryGetAssetInternal(key, MakeTypeId<T>());

		if (assetInternal == nullptr)
		{
			return std::nullopt;
		}

		return WeakAsset<T>{ *assetInternal };
	}

	template <typename T>
	std::shared_ptr<const T> AssetManager::AddAsset(T&& generatedAsset)
	{
		AssetInternal* const assetInternal = TryConstruct(std::nullopt, AssetFileMetaData{ generatedAsset.GetName(), MetaManager::Get().GetType<T>() });

		if (assetInternal == nullptr)
		{
			return nullptr;
		}

		std::shared_ptr<T> ptr = std::make_shared<T>(std::forward<T>(generatedAsset));
		assetInternal->mAsset = ptr;
		return ptr;
	}
}
