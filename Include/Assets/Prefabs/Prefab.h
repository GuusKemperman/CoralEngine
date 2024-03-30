#pragma once
#include "Assets/Asset.h"

#include "Assets/Prefabs/PrefabEntityFactory.h"

namespace CE
{
	class World;

	/*
	A collection of entities and components that can be placed into the world.
	Any changes to the original prefab will be applied to all the instances
	of the prefab in the level.
	
	Based on Unity's prefabs.
	 */
	class Prefab final :
		public Asset
	{
	public:
		Prefab(std::string_view name);
		Prefab(AssetLoadInfo& loadInfo);
		Prefab(Prefab&& other) noexcept;

		~Prefab() override;

		void CreateFromEntity(World& world, entt::entity entity);

		const PrefabEntityFactory* TryFindFactory(uint32 factoryId) const;

		const std::vector<PrefabEntityFactory>& GetFactories() const { return mFactories; }

	private:
		friend class PrefabImporter;

		void OnSave(AssetSaveInfo& saveInfo) const override;
		static void OnSave(AssetSaveInfo& saveInfo, const std::string& prefabName, World& world, entt::entity entity, std::optional<uint32> factorySeed = std::nullopt);

		void LoadFromGSON(BinaryGSONObject& gsonObject);
		static BinaryGSONObject SaveToGSONObject(const std::string& prefabName, World& world, entt::entity entity, std::optional<uint32> factorySeed = std::nullopt);

		std::vector<PrefabEntityFactory> mFactories{};

		uint32 mFactoryIdSeed{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Prefab);
	};
}
