#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;
}

namespace Game
{
	class IdleState
	{
	public:
		static void OnAiTick(CE::World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const CE::World& world, entt::entity owner);
		static void OnAIStateEnterEvent(CE::World& world, entt::entity owner);

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(IdleState);
	};
}
