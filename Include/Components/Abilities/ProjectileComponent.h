#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class ProjectileComponent
	{
	public:
		float mRange{}; // how far the projectile will travel before it gets destroyed
		float mCurrentRange{}; // how far the projectile has traveled

		float mSpeed{}; // initial speed to set the velocity of the physics body

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ProjectileComponent);
	};
}
