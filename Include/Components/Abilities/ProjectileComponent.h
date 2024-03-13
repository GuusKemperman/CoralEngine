#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Engine
{
	class ProjectileComponent
	{
	public:
		float mRange{};
		float mCurrentRange{};

		float mSpeed{};
		float mCurrentSpeed{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ProjectileComponent);
	};
}
