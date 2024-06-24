#include "Precomp.h"
#include "Systems/UtilityAiSystem.h"

#include <entt/entity/runtime_view.hpp>

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

		const std::optional<BoundEvent> aiTickEvent = TryGetEvent(*currentAIController.mCurrentState, sAITickEvent);

		if (!aiTickEvent.has_value())
		{
			continue;
		}

		if (aiTickEvent->mIsStatic)
		{
			aiTickEvent->mFunc.get().InvokeUncheckedUnpacked(world, entity, dt);
		}
		else
		{
			MetaAny component{ *currentAIController.mCurrentState, storage->value(entity), false };
			aiTickEvent->mFunc.get().InvokeUncheckedUnpacked(component, world, entity, dt);
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
		currentAIController.mCurrentScore = std::numeric_limits<float>::lowest();
		currentAIController.mPreviousState = currentAIController.mCurrentState;
		currentAIController.mNextState = nullptr;

#ifdef EDITOR
		currentAIController.mDebugPreviouslyEvaluatedScores.clear();
#endif 
	}

	for (const BoundEvent& boundEvent : mBoundEvaluateEvents)
	{
		entt::sparse_set* const storage = reg.Storage(boundEvent.mType.get().GetTypeId());

		if (storage == nullptr)
		{
			continue;
		}

		entt::runtime_view view{};
		view.iterate(*storage);
		view.iterate(reg.Storage<EnemyAiControllerComponent>());

		for (entt::entity entity : view)
		{
			float score{};

			if (boundEvent.mIsStatic)
			{
				boundEvent.mFunc.get().InvokeUncheckedUnpackedWithRVO(&score, world, entity);
			}
			else
			{
				MetaAny component{ boundEvent.mType, storage->value(entity), false };
				boundEvent.mFunc.get().InvokeUncheckedUnpackedWithRVO(&score, component, world, entity);
			}

			EnemyAiControllerComponent& aiController = reg.Get<EnemyAiControllerComponent>(entity);

			if (score > aiController.mCurrentScore)
			{
				aiController.mCurrentScore = score;
				aiController.mNextState = &boundEvent.mType.get();
			}

#ifdef EDITOR
			aiController.mDebugPreviouslyEvaluatedScores.emplace_back(boundEvent.mType.get().GetName(), score);
#endif
		}
	}

	for (auto [entity, currentAIController] : enemyAIControllerView.each())
	{
#ifdef EDITOR
		std::sort(currentAIController.mDebugPreviouslyEvaluatedScores.begin(), currentAIController.mDebugPreviouslyEvaluatedScores.end(),
			[](const std::pair<std::string_view, float>& lhs, const std::pair<std::string_view, float>& rhs)
			{
				return lhs.second > rhs.second;
			});
#endif
		currentAIController.mCurrentState = currentAIController.mNextState;

		if (currentAIController.mCurrentState != currentAIController.mPreviousState)
		{
			CallTransitionEvent(sAIStateExitEvent, currentAIController.mPreviousState, world, entity);
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

	const std::optional<BoundEvent> boundEvent = TryGetEvent(*type, event);

	if (!boundEvent.has_value())
	{
		return;
	}

	entt::sparse_set* storage = world.GetRegistry().Storage(type->GetTypeId());

	if (storage == nullptr
		|| !storage->contains(owner))
	{
		return;
	}

	if (boundEvent->mIsStatic)
	{
		boundEvent->mFunc.get().InvokeUncheckedUnpacked(world, owner);
	}
	else
	{
		MetaAny component{ *type, storage->value(owner), false };
		boundEvent->mFunc.get().InvokeUncheckedUnpacked(component, world, owner);
	}
}

CE::MetaType CE::AIEvaluateSystem::Reflect()
{
	return MetaType{ MetaType::T<AIEvaluateSystem>{}, "AIEvaluateSystem", MetaType::Base<System>{} };
}
