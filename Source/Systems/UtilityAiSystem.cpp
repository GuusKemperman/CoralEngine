#include "Precomp.h"
#include "Systems/UtilityAiSystem.h"

#include "Components/UtilityAi/EnemyAiControllerComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"

void CE::AITickSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();
	const auto& enemyAIControllerView = reg.View<EnemyAiControllerComponent>();

	for (auto [entity, currentAIController] : enemyAIControllerView.each())
	{
		if (currentAIController.mCurrentState == nullptr)
		{
			continue;
		}

		entt::sparse_set* const storage = reg.Storage(currentAIController.mCurrentState->GetTypeId());

		// Check if your state component still exists
		if (storage == nullptr
			|| !storage->contains(entity))
		{
			continue;
		}

		const MetaFunc* const aiTickEvent = TryGetEvent(*currentAIController.mCurrentState, sAITickEvent);

		if (aiTickEvent == nullptr)
		{
			continue;
		}

		if (aiTickEvent->GetProperties().Has(Props::sIsEventStaticTag))
		{
			aiTickEvent->InvokeUncheckedUnpacked(world, entity, dt);
		}
		else
		{
			MetaAny component{ *currentAIController.mCurrentState, storage->value(entity), false };
			aiTickEvent->InvokeUncheckedUnpacked(component, world, entity, dt);
		}
	}
}

CE::MetaType CE::AITickSystem::Reflect()
{
	return MetaType{ MetaType::T<AITickSystem>{}, "AITickSystem", MetaType::Base<System>{} };
}

void CE::AIEvaluateSystem::Update(World& world, float)
{
	Registry& reg = world.GetRegistry();

	const auto& enemyAIControllerView = world.GetRegistry().View<EnemyAiControllerComponent>();

	for (auto [entity, currentAIController] : enemyAIControllerView.each())
	{
		float bestScore = std::numeric_limits<float>::lowest();
		const MetaType* bestType = nullptr;

#ifdef EDITOR
		currentAIController.mDebugPreviouslyEvaluatedScores.clear();
#endif 

		for (const BoundEvent& boundEvent : mBoundEvaluateEvents)
		{
			entt::sparse_set* const storage = reg.Storage(boundEvent.mType.get().GetTypeId());

			if (storage == nullptr
				|| !storage->contains(entity))
			{
				continue;
			}

			float score{};
			FuncResult evalResult;

			if (boundEvent.mIsStatic)
			{
				evalResult = boundEvent.mFunc.get().InvokeUncheckedUnpackedWithRVO(&score, world, entity);
			}
			else
			{
				MetaAny component{ boundEvent.mType, storage->value(entity), false };
				evalResult = boundEvent.mFunc.get().InvokeUncheckedUnpackedWithRVO(&score, component, world, entity);
			}

			if (evalResult.HasError())
			{
				continue;
			}

#ifdef EDITOR
			currentAIController.mDebugPreviouslyEvaluatedScores.emplace_back(boundEvent.mType.get().GetName(), score);
#endif 

			if (score > bestScore)
			{
				bestScore = score;
				bestType = &boundEvent.mType.get();
			}
		}

#ifdef EDITOR
		std::sort(currentAIController.mDebugPreviouslyEvaluatedScores.begin(), currentAIController.mDebugPreviouslyEvaluatedScores.end(),
			[](const std::pair<std::string_view, float>& lhs, const std::pair<std::string_view, float>& rhs)
			{
				return lhs.second > rhs.second;
			});
#endif

		if (currentAIController.mCurrentState != bestType)
		{
			CallTransitionEvent(sAIStateExitEvent, currentAIController.mCurrentState, world, entity);
			currentAIController.mCurrentState = bestType;
			CallTransitionEvent(sAIStateEnterEvent, currentAIController.mCurrentState, world, entity);
		}
	}
}

template <typename EventT>
void CE::AIEvaluateSystem::CallTransitionEvent(const EventT& event, const MetaType* type, World& world,
	entt::entity owner)
{
	if (type == nullptr)
	{
		return;
	}

	const MetaFunc* const boundEvent = TryGetEvent(*type, event);

	if (boundEvent == nullptr)
	{
		return;
	}

	entt::sparse_set* storage = world.GetRegistry().Storage(type->GetTypeId());

	if (storage == nullptr
		|| !storage->contains(owner))
	{
		return;
	}

	if (boundEvent->GetProperties().Has(Props::sIsEventStaticTag))
	{
		boundEvent->InvokeUncheckedUnpacked(world, owner);
	}
	else
	{
		MetaAny component{ *type, storage->value(owner), false };
		boundEvent->InvokeUncheckedUnpacked(component, world, owner);
	}
}

CE::MetaType CE::AIEvaluateSystem::Reflect()
{
	return MetaType{ MetaType::T<AIEvaluateSystem>{}, "AIEvaluateSystem", MetaType::Base<System>{} };
}
