#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Game
{
	struct XPOrbManagerComponent
	{
		float mXPValue = 1.0f;
		float mChaseRange = 35.0f;
		float mChaseSpeed = 25.0f;
		float mRotationSpeed = 1.0f;
		float mHoverSpeed = 2.0f;
		float mHoverHeight = .1f;
		float mMaxTimeAlive = 30.0f;

		uint32 mMaxAlive = 250;

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(XPOrbManagerComponent);
	};
}
