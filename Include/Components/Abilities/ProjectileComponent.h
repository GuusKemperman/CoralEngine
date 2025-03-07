#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;

	class ProjectileComponent
	{
	public:
#ifdef EDITOR
		static void OnInspect(World& world, entt::entity owner);
#endif // EDITOR

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

		// If the projectile shouldn't follow the exact character direction, but instead be offset to the side.
		float mDirectionOffsetAngle{}; // Euler angle.

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ProjectileComponent);
	};
}
