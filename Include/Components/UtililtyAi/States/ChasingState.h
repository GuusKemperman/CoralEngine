#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;
	class World;

	class ChasingState
	{
	public:
		void OnAiTick(World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const World& world, entt::entity owner) const;
		void OnAIStateEnterEvent(const World& world, entt::entity owner);

		[[nodiscard]] std::pair<float, entt::entity> GetHighestScore(const World& world, entt::entity owner) const;

		void DebugRender(World& world, entt::entity owner) const;

	private:
		entt::entity mTargetEntity = entt::null;
		entt::entity mChosenTargetEntity = entt::null;

		float mRadius = 0;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ChasingState);
	};
}
