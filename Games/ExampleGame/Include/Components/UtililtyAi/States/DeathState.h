#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Events.h"

namespace CE
{
	class Prefab;
	class Animation;
	class World;
}

namespace Game
{

	class DeathState
	{
	public:
		void OnTick(CE::World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const CE::World& world, entt::entity owner);
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner);
		void OnFinishAnimationEvent(CE::World& world, entt::entity owner);

		CE::AssetHandle<CE::Animation> mDeathAnimation{};

		float mCurrentDeathTimer = 0.0f;

		float mLightFadeOutDuration = 1.0f;

		CE::AssetHandle<CE::Prefab> mExpOrb{};

		float mAnimationSpeed = 1.0f;

		float mAnimationStartTimePercentage = .3f;

	private:
		bool mHasStateBeenEntered = false;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DeathState)
	};

}