#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"

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
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const CE::World& world, entt::entity owner);
		void OnAIStateEnterEvent(CE::World& world, entt::entity owner) const;

		CE::AssetHandle<CE::Animation> mDeathAnimation{};

		float mCurrentDeathTimer = 0.0f;

		bool mDestroyEntityWhenDead = false;

		float mMaxDeathTime = 5.0f;

	private:

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DeathState)
	};

}