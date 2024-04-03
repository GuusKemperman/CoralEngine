#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;

	class AOEComponent // Area Of Attack
	{
	public:
		// If you want an AOE that moves you can combine the AOE Component with
		// the Projectile Component. We need this boolean so that the AOE does not get 
		// destroyed based on its duration (lifetime), but instead uses the projectile range.
		bool mUsesProjectileComponent = false;

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
