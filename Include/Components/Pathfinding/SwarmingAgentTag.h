#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;

	class SwarmingAgentTag
	{
	public:
		static void StartMovingToTarget(World& world, entt::entity entity);
		static void StopMovingToTarget(World& world, entt::entity entity);

	private:

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(SwarmingAgentTag);
	};
}
