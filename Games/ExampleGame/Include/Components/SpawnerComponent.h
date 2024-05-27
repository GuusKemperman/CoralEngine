#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class Prefab;
}

namespace Game
{
	class SpawnerComponent
	{
	public:
		// The range in which the player has to be for the spawner to be active.
		float mMinSpawnRange = 50.0f;

		float mSpacing = 5.0f;

		CE::AssetHandle<CE::Prefab> mPrefabToSpawn{};
		float mAmountToSpawnPerSecond = 1.0f;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerComponent);
	};
}
