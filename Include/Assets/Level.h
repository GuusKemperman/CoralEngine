#pragma once
#include "Assets/Asset.h"

#include "GSON/GSONBinary.h"
#include "Assets/Prefabs/Prefab.h"
#include "Assets/Prefabs/PrefabEntityFactory.h"

namespace Engine
{
	class World;
	class Prefab;

	/*
	A level is a way for you to serialize and deserialize world.

	It takes into account any changes that have been made to any
	of the prefabs inside of the world.

	After the world has been deserialized, you don't need to keep
	the level loaded.
	*/
	class Level final :
		public Asset
	{
	public:
		Level(std::string_view name);
		Level(AssetLoadInfo& loadInfo);
		~Level() override;

		Level(Level&& other) noexcept;
		Level(const Level&) = delete;

		Level& operator=(Level&&) = delete;
		Level& operator=(const Level&) = delete;

		void CreateFromWorld(const World& world);
		World CreateWorld(bool callBeginPlayImmediately) const;

	protected:
		void OnSave(AssetSaveInfo& saveInfo) const override;

	private:
		struct DiffedPrefabFactory
		{
			std::reference_wrapper<const PrefabEntityFactory> mCurrentFactory;
			std::reference_wrapper<const BinaryGSONObject> mSerializedFactory;
			std::vector<std::reference_wrapper<const ComponentFactory>> mAddedComponents{};
			std::vector<std::string> mNamesOfRemovedComponents{};
		};
		DiffedPrefabFactory DiffPrefabFactory(const BinaryGSONObject& serializedFactory, const PrefabEntityFactory& currentVersion);

		struct DiffedPrefab
		{
			std::string mPrefabName{};

			std::optional<std::shared_ptr<const Prefab>> mPrefab{};

			// The factories that have been added to the prefab while our level was stored in a file offline.
			// This vector is sorted so that when iterating over them, you are guaranteed to encounter each
			// parent before any of it's children
			std::vector<std::reference_wrapper<const PrefabEntityFactory>> mAddedFactories{};
			std::vector<std::reference_wrapper<const BinaryGSONObject>> mSerializedFactoriesThatWereRemoved{};
			std::vector<DiffedPrefabFactory> mDifferencesBetweenExistingFactories{};
		};
		DiffedPrefab DiffPrefab(const BinaryGSONObject& serializedPrefab);

		static BinaryGSONObject GenerateCurrentStateOfPrefabs(const BinaryGSONObject& serializedWorld);

		std::unique_ptr<BinaryGSONObject> mSerializedComponents{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Level);
	};
}

