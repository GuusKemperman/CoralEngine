#pragma once
#include "BasicDataTypes/Bezier.h"
#include "Meta/MetaReflect.h"

namespace Game
{
	class DifficultyScalingComponent
	{
	public:
		CE::Bezier mScaleHPOverTime{};
		CE::Bezier mScaleDamageOverTime{};

		bool mIsRepeating = false;
		unsigned int mLoopsElapsed = 0;

		// In seconds
		float mScaleTime = 60.0f;

		float mMinHealthMultiplier = 1.0f;
		float mMaxHealthMultiplier = 5.0f;

		float mMinDamageMultiplier = 1.0f;
		float mMaxDamageMultiplier = 5.0f;

		float mCurrentHealthMultiplier{};
		float mCurrentDamageMultiplier{};

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DifficultyScalingComponent);
	};
}
