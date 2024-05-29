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

		bool mDoesRepeat = false;

		// In seconds
		float mScaleLength = 60.0f;

		float mMinHPMultiplier = 1.0f;
		float mMaxHPMultiplier = 5.0f;

		float mMinDamageMultiplier = 1.0f;
		float mMaxDamageMultiplier = 5.0f;

		float mCurrentHPMultiplier{};
		float mCurrentDamageMultiplier{};

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DifficultyScalingComponent);
	};
}
