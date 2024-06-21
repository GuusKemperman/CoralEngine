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

		static void AddShake(CE::World& world, float rangeX = 0.5f, float rangeY = 0.5f, float duration = 5.f, float speed = 10.f);

		float mFadeOutAtIntensity = 3.f;

		struct ShakeEffect
		{
			float mTimeLeft{};
			float mShakeSpeed = 10.f;
			float mRangeX = .5f;
			float mRangeY = .5f;
		};
		std::vector<ShakeEffect> mActiveEffects{};

	private:

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(CameraShakeComponent);
	};

}
