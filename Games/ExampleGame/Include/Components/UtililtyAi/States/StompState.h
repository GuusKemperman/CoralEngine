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

		CE::Cooldown mStompCooldown{};

	private:
		float mRadius{};

		float mMaxStompTime = 10.0f;

		CE::AssetHandle<CE::Prefab> mParticles{};

		entt::entity mSpawnedVfx;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(StompState);
	};

}