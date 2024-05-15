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
		void OnAiTick(CE::World& world, entt::entity owner, float dt);
		float OnAiEvaluate(const CE::World& world, entt::entity owner);

		[[nodiscard]] std::pair<float, entt::entity> GetBestScoreAndTarget(const CE::World& world,
		                                                             entt::entity owner) const;

		void DebugRender(CE::World& world, entt::entity owner) const;

		CE::AssetHandle<CE::Animation> mChasingAnimation{};

	private:
		entt::entity mTargetEntity = entt::null;
		float mRadius{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(ChasingState);
	};
}
