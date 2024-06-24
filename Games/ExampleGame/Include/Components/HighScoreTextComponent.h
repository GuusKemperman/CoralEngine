#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;
}

namespace Game
{
	class HighScoreTextComponent
	{
	public:
		static void OnTick(CE::World& world, entt::entity owner, float dt);

	private:

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(HighScoreTextComponent);
	};
}
