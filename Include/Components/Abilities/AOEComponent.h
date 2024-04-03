#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;

	class AOEComponent // Area Of Attack
	{
	public:
		// How long the ability should be alive for.
		// You need this even if you want it to be instant
		// so that it stays on screen more than one frame.
		float mDuration{};

		// How long it has been alive for.
		float mDurationTimer{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
#ifdef EDITOR
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);
#endif // EDITOR
		REFLECT_AT_START_UP(AOEComponent);
	};
}
