#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class World;
	class TransformComponent;
}

namespace Game
{
	class ChasingState
	{
	public:
		void OnAiTick(Engine::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const Engine::World& world, entt::entity owner) const;

		[[nodiscard]] std::pair<float, entt::entity> GetBestScoreAndTarget(const Engine::World& world,
		                                                             entt::entity owner) const;

		void DebugRender(Engine::World& world, entt::entity owner) const;

	private:
		entt::entity mTargetEntity{};
		float mRadius{};

		friend Engine::ReflectAccess;
		static Engine::MetaType Reflect();
		REFLECT_AT_START_UP(ChasingState);
	};
}
