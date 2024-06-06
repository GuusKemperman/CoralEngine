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
		void OnAiTick(CE::World& world, entt::entity owner, float dt) const;
		static float OnAiEvaluate(const CE::World& world, entt::entity owner);
		static void OnAiStateEnterEvent(CE::World& world, entt::entity owner);

		CE::AssetHandle<CE::Animation> mIdleAnimation{};

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(IdleState);
	};
}
