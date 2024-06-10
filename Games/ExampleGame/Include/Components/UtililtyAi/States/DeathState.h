#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Events.h"

namespace CE
{
	class Animation;
	class World;
}

namespace Game
{

	class DeathState
	{
	public:
		DeathState();

		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const CE::World& world, entt::entity owner);
		void OnAIStateEnterEvent(CE::World& world, entt::entity owner) const;

		CE::AssetHandle<CE::Animation> mDeathAnimation{};

		float mCurrentDeathTimer = 0.0f;

		bool mDestroyEntityWhenDead = false;

		float mMaxDeathTime = 5.0f;

		float mAnimationBlendTime = 1.5f;

	private:
		static inline std::vector<CE::BoundEvent> sEnemyKilledEvents;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DeathState)
	};

}