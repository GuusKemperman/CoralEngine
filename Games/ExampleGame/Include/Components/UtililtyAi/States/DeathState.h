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
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const CE::World& world, entt::entity owner);
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner) const;
		void OnFinishAnimationEvent(CE::World& world, entt::entity owner);

		CE::AssetHandle<CE::Animation> mDeathAnimation{};

		float mCurrentDeathTimer = 0.0f;

		bool mDestroyEntityWhenDead = true;

		float mMaxDeathTime = 0.5f;

		float mAnimationBlendTime = 1.5f;

		CE::AssetHandle<CE::Prefab> mExpOrb{};

	private:

		bool mSink = false;

		float mSinkDownSpeed = 0.02f;
		float mSinkSizeDown = 0.99f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DeathState)
	};

}