#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class Animation;
	class World;
	class TransformComponent;
}

namespace Game
{
	class KnockBackState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner);
		static void OnAiStateExitEvent(CE::World& world, entt::entity owner);

		void ResetTimer();

		float mCurrentKnockBackCountDownTimer = 0.0f;

		CE::AssetHandle<CE::Animation> mKnockBackAnimation{};

	private:

		glm::vec2 mDashDirection{};

		float mKnockBackSpeed{};
		float mMaxKnockBackTime = 1.0f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(KnockBackState);
	};
}