#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class PrefabEntityFactory;
	class Prefab;
	class World;
	class BinaryGSONObject;

	class PrefabOriginComponent
	{
		friend class Prefab;
		PrefabOriginComponent(Name::HashType hashedPrefabName, uint32 factoryId);
	public:
		PrefabOriginComponent() = default;
		PrefabOriginComponent(const PrefabEntityFactory& factory);

		void SetFactoryOfOrigin(const PrefabEntityFactory& factory);

		AssetHandle<Prefab> TryGetPrefab() const;
		const PrefabEntityFactory* TryGetFactory() const;

		Name::HashType GetHashedPrefabName() const { return mHashedPrefabName; }
		uint32 GetFactoryId() const { return mFactoryId; }

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PrefabOriginComponent);

		Name::HashType mHashedPrefabName{};
		uint32 mFactoryId{};
	};
}
