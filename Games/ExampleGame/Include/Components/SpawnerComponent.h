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
		// The range in which the player has to be for the spawner to be active.
		float mMin = 5.0f;
		float mMax = 10.0f;

		bool mActive = false;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(SpawnerComponent);
	};
}
