#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class Prefab;

	class ParticleSpawnPrefabOnDeathComponent
	{
	public:
		AssetHandle<Prefab> mPrefabToSpawn{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleSpawnPrefabOnDeathComponent);
	};
}
