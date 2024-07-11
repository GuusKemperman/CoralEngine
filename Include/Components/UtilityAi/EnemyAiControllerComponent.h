#pragma once
#include "Systems/System.h"

namespace CE
{
	class EnemyAiControllerComponent
	{
	public:
		const MetaType* mPreviousState{};
		const MetaType* mCurrentState{};
		const MetaType* mNextState{};
		float mCurrentScore{};

#ifdef EDITOR
		static void OnInspect(World& world, const std::vector<entt::entity>& entities);

		// Is updated every frame. Used in the custom inspect event to give the user
		// more information. The string is the name of the type.
		std::vector<std::pair<std::string_view, float>> mDebugPreviouslyEvaluatedScores{};
#endif

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(EnemyAiControllerComponent);
	};
}
