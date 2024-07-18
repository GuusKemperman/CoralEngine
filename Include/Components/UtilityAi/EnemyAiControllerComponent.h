#pragma once
#include "Utilities/Events.h"

namespace CE
{
	struct OnAIEvaluate :
		EventType<OnAIEvaluate, float()>
	{
		OnAIEvaluate() :
			EventType("OnAIEvaluate", "EvaluationScore")
		{
		}
	};
	inline const OnAIEvaluate sOnAIEvaluate{};

	struct OnAIStateEnter :
		EventType<OnAIStateEnter>
	{
		OnAIStateEnter() :
			EventType("OnAIStateEnter")
		{
		}
	};
	inline const OnAIStateEnter sOnAIStateEnter{};

	struct OnAITick :
		EventType<OnAITick, void(float)>
	{
		OnAITick() :
			EventType("OnAITick", "DeltaTime")
		{
		}
	};
	inline const OnAITick sOnAITick{};

	struct OnAIStateExit :
		EventType<OnAIStateExit>
	{
		OnAIStateExit() :
			EventType("OnAIStateExit")
		{
		}
	};
	inline const OnAIStateExit sOnAIStateExit{};

	class EnemyAiControllerComponent
	{
	public:
		const MetaType* mPreviousState{};
		const MetaType* mCurrentState{};
		const MetaType* mNextState{};
		float mCurrentScore{};

#ifdef EDITOR
		void OnInspect(World& world, entt::entity owner);

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
