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

	class StompExecutionState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAIStateEnterEvent(CE::World& world, entt::entity owner);

		bool IsStompCharged() const;

		[[nodiscard]] std::pair<float, entt::entity> GetBestScoreAndTarget(const CE::World& world,
			entt::entity owner) const;

		float mCurrentStompTimer = 0.0f;

		CE::AssetHandle<CE::Animation> mStompAnimation{};

	private:
		float mRadius{};

		float mMaxStompTime = 10.0f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(StompExecutionState);
	};

}