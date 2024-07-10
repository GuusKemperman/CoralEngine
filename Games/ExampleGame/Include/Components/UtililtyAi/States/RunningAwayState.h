#pragma once
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Animation;
	class World;
}

namespace Game
{

	class RunningAwayState
	{
	public:
		void OnAiStateEnter(CE::World& world, entt::entity owner);
		void OnAiStateExit(CE::World& world, entt::entity owner);

		void OnAiTick(CE::World& world, entt::entity owner, float dt) const;

		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnBeginPlay(CE::World& world, entt::entity owner) const;

		void DebugRender(CE::World& world, entt::entity owner) const;

		CE::AssetHandle<CE::Animation> mChasingAnimation{};

		float mStartYAxis = 0.f;

	private:
		float mRunAnimationSpeed = 2.f;

		float mRadius{};
		float mDestroyRadius = 50.f;

		float mUpperSpeedRange = 0.2f;
		float mLowerSpeedRange = -0.2f;

		bool mStartedEscaping = false;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(RunningAwayState);
	};

}
