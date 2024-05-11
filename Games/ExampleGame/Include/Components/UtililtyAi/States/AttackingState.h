#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;
}

namespace Game
{
	class AttackingState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAIStateEnterEvent( CE::World& world, entt::entity owner);

		[[nodiscard]] std::pair<float, entt::entity> GetBestScoreAndTarget(const CE::World& world,
		                                                             entt::entity owner) const;

	private:
		entt::entity mTargetEntity{};
		float mRadius{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(AttackingState);
	};
}
