#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include <Utilities/Time.h>

#include "Assets/Prefabs/Prefab.h"

namespace CE
{
	class Animation;
	class World;
}

namespace Game
{
	class ChargeUpDashState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner);
		void OnAiStateExitEvent(CE::World& world, entt::entity owner);

		bool IsCharged() const;

		CE::AssetHandle<CE::Animation> mChargingAnimation{};
		float mMaxChargeTime = 10.0f;
		float mRadius{};
		CE::AssetHandle<CE::Prefab> mVFX{};

	private:
		entt::entity mSpawnedVFX;
		CE::Cooldown mChargeCooldown{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ChargeUpDashState);
	};
}