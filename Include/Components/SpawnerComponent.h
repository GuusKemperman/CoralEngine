#pragma once
#include "Systems/System.h"

namespace Engine
{
	class Prefab;

	class SpawnerComponent
	{
	public:
		std::shared_ptr<const Prefab> mEnemyPrefab{};

		float mCurrentTimer = 0;
		float mSpawningTimer = 5.00000;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerComponent);
	};
}
