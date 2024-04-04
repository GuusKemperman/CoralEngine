#pragma once
#include "Assets/Prefabs/Prefab.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class ParticleSpawnPrefabOnDeathComponent
	{
	public:
		std::shared_ptr<const Prefab> mPrefabToSpawn{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleSpawnPrefabOnDeathComponent);
	};
}
