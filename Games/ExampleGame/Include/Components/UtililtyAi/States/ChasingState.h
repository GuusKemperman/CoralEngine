#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class World;
	class TransformComponent;
}

namespace Game
{
	class ChasingState
	{
	public:
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner) const;

		[[nodiscard]] std::pair<float, entt::entity> GetBestScoreAndTarget(const CE::World& world,
		                                                             entt::entity owner) const;

		void DebugRender(CE::World& world, entt::entity owner) const;

	private:
		entt::entity mTargetEntity{};
		float mRadius{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ChasingState);
	};
}
