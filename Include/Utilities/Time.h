#pragma once

namespace CE
{
	struct Cooldown
	{
		Cooldown(float cooldown) :
			mCooldown(cooldown)
		{}

		bool IsReady(float dt)
		{
			mAmountOfTimePassed += dt;

			if (mAmountOfTimePassed >= mCooldown)
			{
				mAmountOfTimePassed -= mCooldown;
				return true;
			}
			return false;
		}

		float mCooldown{};
		float mAmountOfTimePassed{};
	};

	struct Timer
	{
		Timer() :
			mStart(std::chrono::high_resolution_clock::now()){}

		float GetSecondsElapsed() const
		{
			return std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - mStart).count();
		}

		void Reset()
		{
			mStart = std::chrono::high_resolution_clock::now();
		}

		std::chrono::high_resolution_clock::time_point mStart{};
	};
}