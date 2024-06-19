#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Assets/Prefabs/Prefab.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Time.h"

namespace CE
{
	class Animation;
	class World;
}

namespace Game
{
	class ChargeUpStompState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		[[nodiscard]] float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAiStateExitEvent(CE::World& world, entt::entity entity);
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner);

		[[nodiscard]] bool IsCharged() const;

		CE::AssetHandle<CE::Animation> mChargingAnimation{};
		float mRadius{};
		float mMaxChargeTime = 10.0f;
		CE::AssetHandle<CE::Prefab> mVFX{};

	private:
		entt::entity mSpawnedVFX;
		float mCurrentTime{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ChargeUpStompState);
	};

}