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
	class ChargeDashState
	{
	public:
		void OnAITick(CE::World& world, entt::entity owner, float dt);
		float OnAIEvaluate(const CE::World& world, entt::entity owner) const;

		bool IsDashCharged() const;

		[[nodiscard]] std::pair<float, entt::entity> GetBestScoreAndTarget(const CE::World& world,
			entt::entity owner) const;

		float mCurrentChargeTimer = 0.0f;

		CE::AssetHandle<CE::Animation> mChargingAnimation{};

	private:
		float mRadius{};

		float mMaxChargeTime = 10.0f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ChargeDashState);
	};
}
