#pragma once

namespace Engine
{
	struct ScalableTimer
	{
		void Step(float realDeltaTime)
		{
			const float timeScale = GetTimeScale();

			mRealDeltaTime = realDeltaTime;
			mScaledDeltaTime = realDeltaTime * timeScale;

			mRealTotalTimeElapsed += realDeltaTime;
			mScaledTotalTimeElapsed += mScaledDeltaTime;
		}

		float GetTimeScale() const { return mIsPaused ? 0.0f : mTimescale; }

		bool mIsPaused{};

		// Use GetTimeScale to take into account pausing
		float mTimescale = 1.0f;

		float mScaledDeltaTime{};
		float mRealDeltaTime{};

		float mRealTotalTimeElapsed{};
		float mScaledTotalTimeElapsed{};
	};
}
