#pragma once
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
		std::shared_ptr<const CE::Prefab> mPrefab{};

		float mCurrentTimer = 0.0f;
		float mSpawningTimer = 5.0f;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerComponent);
	};
}
