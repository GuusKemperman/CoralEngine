#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/PerlinNoise.h"

namespace CE
{
	class World;
}

namespace Game
{

	class CameraShakeComponent
	{
	public:
		void OnTick(CE::World& world, entt::entity owner, float dt);

		static void AddShake(CE::World& world, float intensity, float duration);

		float mShakeSpeed = 10.f;

		float mRange = .5f;

		float mFallOffSpeed = 3.f;

		float mFadeOutAtIntensity = 3.f;

		struct ShakeEffect
		{
			float mTimeLeft{};
			float mIntensity{};
		};
		std::vector<ShakeEffect> mActiveEffects{};

	private:

		float mCurrentIntensity;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(CameraShakeComponent);
	};

}
