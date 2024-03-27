#pragma once
#include "Systems/System.h"

namespace Engine
{
	class Prefab;
}

namespace Game
{
	class SpawnerComponent
	{
	public:
		std::shared_ptr<const Engine::Prefab> mEnemyPrefab{};

		float mCurrentTimer = 0;
		float mSpawningTimer = 5.00000;

	private:
		friend Engine::ReflectAccess;
		static Engine::MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerComponent);
	};
}
