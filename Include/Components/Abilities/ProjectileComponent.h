#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;

	class ProjectileComponent
	{
	public:

		// You may want an ability that gets destroyed based on its lifetime instead.
		bool mDestroyOnRangeReached = true;

		// How far the projectile will travel before it gets destroyed.
		float mRange{};

		// How far the projectile has traveled.
		float mCurrentRange{};

		// Initial speed to set the velocity of the physics body.
		float mSpeed{};

		// How many characters the projectile can affect before getting destroyed.
		int mPierceCount{};

		// How many characters the projectile has affected.
		int mCurrentPierceCount{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
#ifdef EDITOR
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);
#endif // EDITOR
		REFLECT_AT_START_UP(ProjectileComponent);
	};
}
