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
		void OnAiTick(CE::World& world, entt::entity owner, float dt) const;
		[[nodiscard]] float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAiStateEnter(CE::World& world, entt::entity owner);

		CE::AssetHandle<CE::Animation> mAttackingAnimation{};

	private:
		entt::entity mTargetEntity = entt::null;
		float mRadius{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(AttackingState);
	};
}
