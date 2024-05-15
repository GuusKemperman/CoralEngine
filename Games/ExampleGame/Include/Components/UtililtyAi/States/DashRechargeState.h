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
	class DashRechargeState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		static void OnAIStateEnterEvent(CE::World& world, entt::entity owner);

		float mCurrentRechargeTimer = 0.0f;

	private:
		entt::entity mTargetEntity{};

		float mMaxRechargeTime = 5.0f;

		CE::AssetHandle<CE::Animation> mDashRechargeAnimation{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DashRechargeState);
	};
}
