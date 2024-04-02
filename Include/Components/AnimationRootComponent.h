#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class Animation;

	class AnimationRootComponent
	{
	public:

		void SwitchAnimation();
		void SwitchAnimation(const std::shared_ptr<const Animation>& animation, float timeStamp);

		bool mSwitchAnimation = false;
		float mWantedTimeStamp = 0.0f;
		std::shared_ptr<const Animation> mWantedAnimation{};

	private:
		friend ReflectAccess;
        static MetaType Reflect();
        REFLECT_AT_START_UP(AnimationRootComponent);
	};

}