#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class Animation;
	class World;
	class TransformComponent;
}

namespace Game
{
	class ChasingState
	{
	public:
		void OnAiStateEnter(CE::World& world, entt::entity owner) const;
		static void OnAiStateExit(CE::World& world, entt::entity owner);

		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnBeginPlay(CE::World& world, entt::entity owner) const;

		void DebugRender(CE::World& world, entt::entity owner) const;

		CE::AssetHandle<CE::Animation> mChasingAnimation{};

	private:
		float mRadius{};

		float mUpperSpeedRange = 0.2f;
		float mLowerSpeedRange = -0.2f;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ChasingState);
	};
}
