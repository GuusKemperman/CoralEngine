#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Game
{
	struct XPOrbComponent
	{
		float mXPValue = 1.0f;
		float mChaseRange = 5.0f;
		float mChaseSpeed = 5.0f;
		float mRotationSpeed = 1.0f;
		float mHoverTime = 0.0f;
		float mHoverSpeed = 2.0f;
		float mHoverHeight = .1f;
		float mSecondsRemainingUntilDespawn = 120.0f;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(XPOrbComponent);
	};
}
