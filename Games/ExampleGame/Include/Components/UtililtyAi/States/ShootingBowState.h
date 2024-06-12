#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Time.h"

namespace CE
{
	class Animation;
	class World;
}

namespace Game
{

	class ShootingBowState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner);
		void OnAiStateExitEvent(CE::World& world, entt::entity owner);
		void OnFinishAnimationEvent(CE::World& world, entt::entity owner);

		bool IsShootingCharged() const;

		CE::AssetHandle<CE::Animation> mShootingAnimation{};

		CE::Cooldown mShootCooldown{};

	private:
		float mRadius{};

		float mMaxShootTime = 10.0f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ShootingBowState);
	};

}