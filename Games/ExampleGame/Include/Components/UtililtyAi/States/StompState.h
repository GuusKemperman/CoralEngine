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

	class StompState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAIStateEnterEvent(CE::World& world, entt::entity owner);

		bool IsStompCharged() const;

		CE::AssetHandle<CE::Animation> mStompAnimation{};

		CE::Cooldown mStompCooldown{};

	private:
		float mRadius{};

		float mMaxStompTime = 10.0f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(StompState);
	};

}