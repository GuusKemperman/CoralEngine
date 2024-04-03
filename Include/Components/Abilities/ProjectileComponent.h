#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class ProjectileComponent
	{
	public:
		// How far the projectile will travel before it gets destroyed.
		float mRange{};

		// How far the projectile has traveled.
		float mCurrentRange{};

		// Initial speed to set the velocity of the physics body.
		float mSpeed{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ProjectileComponent);
	};
}
