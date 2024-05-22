#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Prefab;
	class World;

	class SpawnPrefabOnDestructComponent
	{
	public:
		void OnDestruct(World& world, entt::entity entity);

		AssetHandle<Prefab> mPrefab{};

	private:
		
		friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(SpawnPrefabOnDestructComponent);
	};
}