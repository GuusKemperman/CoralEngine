#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Engine
{
	class World;

	class AOEComponent // Area Of Attack
	{
	public:
		float mDuration; // how long the ability should be alive - you need this even if you want it to be instant so that it stays on screen more than one frame
		float mDurationTimer; // how long it has been alive for

	private:
		friend ReflectAccess;
		static MetaType Reflect();
#ifdef EDITOR
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);
#endif // EDITOR
		REFLECT_AT_START_UP(AOEComponent);
	};
}
