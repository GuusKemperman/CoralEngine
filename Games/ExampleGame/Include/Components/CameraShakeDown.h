#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/PerlinNoise.h"

namespace CE
{
	class World;
}

namespace Game
{

	class CameraShakeDown
	{
	public:
		void OnTick(CE::World& world, entt::entity owner, float dt);

		void AddShake(float intensity);

		float mShakeSpeed = 1.f;

		float mRange = 10.f;

		float mFallOffSpeed = 1.f;
	private:

		float mCurrentIntensity;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(CameraShakeDown);
	};

}
