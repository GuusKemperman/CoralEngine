#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Time.h"

namespace CE
{
	class Animation;
	class World;
	class TransformComponent;
}

namespace Game
{
	class RecoveryState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		[[nodiscard]] float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner);
		void OnBeginPlayEvent(CE::World& world, entt::entity owner);

		CE::AssetHandle<CE::Animation> mDashRechargeAnimation{};

		CE::Cooldown mRechargeCooldown{};

	private:
		entt::entity mTargetEntity = entt::null;

		float mMaxRechargeTime = 5.0f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(RecoveryState);
	};
}
