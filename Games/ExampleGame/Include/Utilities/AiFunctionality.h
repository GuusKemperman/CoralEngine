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

	[[nodiscard]] float GetBestScoreBasedOnDetection(const CE::World& world,
		entt::entity owner, float radius);

	void ExecuteEnemyAbility(CE::World& world, entt::entity owner);

	void AnimationInAi(CE::World& world, entt::entity owner, const CE::AssetHandle<CE::Animation>& animation, bool startAtRandomTime);

	void FaceThePlayer(CE::World& world, entt::entity owner);
}
