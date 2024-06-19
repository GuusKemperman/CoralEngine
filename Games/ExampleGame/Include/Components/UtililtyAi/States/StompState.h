#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Time.h"

namespace CE
{
	class Prefab;
	class Animation;
	class World;
}

namespace Game
{

	class StompState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		[[nodiscard]] float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner);

		[[nodiscard]] bool IsStompCharged() const;

		CE::AssetHandle<CE::Animation> mStompAnimation{};
		float mRadius{};
		float mMaxStompTime = 10.0f;
		CE::AssetHandle<CE::Prefab> mVFX{};

	private:
		CE::Cooldown mStompCooldown{};
		entt::entity mSpawnedVFX{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(StompState);
	};

}