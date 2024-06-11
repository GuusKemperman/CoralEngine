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

	class DanceState
	{
	public:
		static void OnAiTick(CE::World& world, entt::entity owner, float dt);
		static float OnAiEvaluate(const CE::World& world, entt::entity owner);
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner) const;

		CE::AssetHandle<CE::Animation> mDanceAnimation{};

	private:

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(DanceState)
	};

}