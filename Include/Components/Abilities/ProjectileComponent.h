#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Engine
{
	class ProjectileComponent
	{
	public:
		float mRange{};
		float mCurrentRange{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ProjectileComponent);
	};
}
