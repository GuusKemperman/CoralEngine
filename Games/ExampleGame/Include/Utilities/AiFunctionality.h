#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Time.h"

namespace CE
{
	class AnimationRootComponent;
	class Animation;
	class World;
}

namespace Game {

	class AIFunctionality
	{
	public:
		[[nodiscard]] static float GetBestScoreBasedOnDetection(const CE::World& world,
			entt::entity owner, float radius);

		static void ExecuteEnemyAbility(CE::World& world, entt::entity owner);

		static void AnimationInAi(CE::World& world, entt::entity owner, const CE::AssetHandle<CE::Animation>& animation, bool startAtRandomTime);

		static void FaceThePlayer(CE::World& world, entt::entity owner);

	private:
		friend CE::ReflectAccess;
		static CE::MetaType Reflect();
		REFLECT_AT_START_UP(AIFunctionality);
	};
}
