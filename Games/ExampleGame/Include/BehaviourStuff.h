#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Time.h"

namespace CE
{
	class Animation;
	class World;
}

namespace Game {

	[[nodiscard]] float GetBestScoreBasedOnDetection(const CE::World& world,
		entt::entity owner, float radius);

}
