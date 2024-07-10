#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Game
{
	struct XPOrbComponent
	{
		float mTimeAlive = 0.0f;
	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(XPOrbComponent);
	};
}
