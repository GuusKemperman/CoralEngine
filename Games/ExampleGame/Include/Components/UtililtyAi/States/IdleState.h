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
	class IdleState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const CE::World& world, entt::entity owner);
		static void OnAIStateEnterEvent(CE::World& world, entt::entity owner);

	private:
		CE::AssetHandle<CE::Animation> mIdleAnimation;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(IdleState);
	};
}
