#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class World;
}

namespace Game
{
	class IdleState
	{
	public:
		static void OnAiTick(Engine::World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const Engine::World& world, entt::entity owner);
		static void OnAIStateEnterEvent(Engine::World& world, entt::entity owner);

	private:
		friend Engine::ReflectAccess;
		static Engine::MetaType Reflect();
		REFLECT_AT_START_UP(IdleState);
	};
}
