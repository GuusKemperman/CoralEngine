#pragma once
#include "Assets/Prefabs/ComponentFactory.h"

namespace CE
{
	class Prefab;
	class World;

	class PrefabEntityFactory
	{
	public:
		PrefabEntityFactory(Prefab& prefab,
			const BinaryGSONObject& allSerializedComponents,
			entt::entity entityIdInSerializedComponents,
			uint32 factoryId,
			const PrefabEntityFactory* parent,
			std::vector<std::reference_wrapper<const PrefabEntityFactory>>&& children);

		PrefabEntityFactory(const PrefabEntityFactory&) = delete;
		PrefabEntityFactory(PrefabEntityFactory&&) noexcept = default;

		PrefabEntityFactory& operator=(PrefabEntityFactory&&) noexcept = delete;
		PrefabEntityFactory& operator=(const PrefabEntityFactory&) = delete;

		const std::vector<ComponentFactory>& GetComponentFactories() const { return mComponentFactories; }
		const ComponentFactory* TryGetComponentFactory(const MetaType& objectClass) const;

		Prefab& GetPrefab() { return mPrefab; }
		const Prefab& GetPrefab() const { return mPrefab; }

		const PrefabEntityFactory* GetParent() const { return mParent; }
		const std::vector<std::reference_wrapper<const PrefabEntityFactory>>& GetChildren() const { return mChildren; }

		uint32 GetId() const { return mId; }

	private:
		std::reference_wrapper<Prefab> mPrefab; friend Prefab; // Friends because in Prefab(Prefab&&) the address is updated.
		const PrefabEntityFactory* mParent{};

		std::vector<std::reference_wrapper<const PrefabEntityFactory>> mChildren{};
		std::vector<ComponentFactory> mComponentFactories{};

		// Not the same as index!
		uint32 mId{};

	};
}