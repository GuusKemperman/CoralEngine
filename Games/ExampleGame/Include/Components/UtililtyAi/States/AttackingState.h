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
	class AttackingState
	{
	public:
		void OnAITick(CE::World& world, entt::entity owner, float dt);
		float OnAIEvaluate(const CE::World& world, entt::entity owner);

		[[nodiscard]] std::pair<float, entt::entity> GetBestScoreAndTarget(const CE::World& world,
		                                                             entt::entity owner) const;

		CE::AssetHandle<CE::Animation> mAttackingAnimation{};

	private:
		entt::entity mTargetEntity = entt::null;
		float mRadius{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(AttackingState);
	};
}
