#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;
}

namespace Game
{
	struct CorpseComponent
	{
		void OnBeginPlay(const CE::World& world, entt::entity owner);

		float mTimeOfDeath = 0.0f;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(CorpseComponent);
	};
}
