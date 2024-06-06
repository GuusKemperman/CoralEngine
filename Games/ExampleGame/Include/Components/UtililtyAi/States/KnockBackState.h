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
	class KnockBackState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;
		void OnAiStateEnterEvent(CE::World& world, entt::entity owner);
		static void OnAiStateExitEvent(CE::World& world, entt::entity owner);

		void Initialize(float knockbackValue);

		CE::AssetHandle<CE::Animation> mKnockBackAnimation{};

	private:

		glm::vec2 mDashDirection{};

		float mKnockBackSpeed{};
		float mFriction = 0.99f;
		float mMinKnockBackSpeed;

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(KnockBackState);
	};
}