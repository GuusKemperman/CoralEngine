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

		float mCurrentRechargeTimer = 0.0f;

		CE::AssetHandle<CE::Animation> mDashRechargeAnimation{};

	private:
		entt::entity mTargetEntity = entt::null;

		float mMaxRechargeTime = 5.0f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DashRechargeState);
	};
}
