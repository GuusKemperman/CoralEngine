#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class World;

	class ChasingState
	{
	public:
		static void OnAiTick(World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const World& world, entt::entity owner);

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ChasingState);
	};
}
