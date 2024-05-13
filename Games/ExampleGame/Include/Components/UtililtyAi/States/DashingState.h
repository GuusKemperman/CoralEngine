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
	class DashingState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		static void OnAIStateEnterEvent(CE::World& world, entt::entity owner);

		bool IsDashCharged() const;

		[[nodiscard]] std::pair<float, entt::entity> GetBestScoreAndTarget(const CE::World& world,
			entt::entity owner) const;

		float mCurrentDashTimer = 0.0f;

	private:
		entt::entity mTargetEntity{};
		float mRadius{};

		float mSpeedDash{};
		float mMaxDashTime = 1.0f;

		CE::AssetHandle<CE::Animation> mDashingAnimation{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DashingState);
	};
}
