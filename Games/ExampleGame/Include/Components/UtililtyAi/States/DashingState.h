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
	class DashingState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAIStateEnterEvent(CE::World& world, entt::entity owner);

		bool IsDashCharged() const;

		CE::AssetHandle<CE::Animation> mDashingAnimation{};

		CE::Cooldown mDashCooldown;

	private:
		entt::entity mTargetEntity = entt::null;

		glm::vec2 mDashDirection{};

		float mSpeedDash{};
		float mMaxDashTime = 1.0f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DashingState);
	};
}
