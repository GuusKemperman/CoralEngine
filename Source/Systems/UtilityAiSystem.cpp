#include "Precomp.h"
#include "Systems/UtilityAiSystem.h"

#include "Components/UtilityAi/EnemyAiControllerComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"

void Engine::AITickSystem::Update(World& world, float dt)
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
			LOG(LogAI, Warning, "Component {} has an {} event, but not a {} event",
			    currentAIController.mCurrentState->GetName(),
			    sAIEvaluateEvent.mName,
			    sAITickEvent.mName);
			continue;
		}

		if (aiTickEvent->GetProperties().Has(Props::sIsEventStaticTag))
		{
			aiTickEvent->InvokeUncheckedUnpacked(world, entity, dt);
		}
		else
		{
			MetaAny component{*currentAIController.mCurrentState, storage->value(entity), false};
			aiTickEvent->InvokeUncheckedUnpacked(component, world, entity, dt);
		}
	}
}

Engine::MetaType Engine::AITickSystem::Reflect()
{
	return MetaType{MetaType::T<AITickSystem>{}, "AITickSystem", MetaType::Base<System>{}};
}

void Engine::AIEvaluateSystem::Update(World& world, float)
{
	const Registry& reg = world.GetRegistry();

	struct StateToEvaluate
	{
		std::reference_wrapper<const entt::sparse_set> mStorage;
		std::reference_wrapper<const MetaType> mType;
		std::reference_wrapper<const MetaFunc> mEvaluate;
		bool mAreEventsStatic{};
	};
	std::vector<StateToEvaluate> statesToEvaluate{};

	for (const auto&& [typeId, storage] : reg.Storage())
	{
		const MetaType* const state = MetaManager::Get().TryGetType(typeId);

		if (state == nullptr)
		{
			continue;
		}

		const MetaFunc* const evaluateEvent = TryGetEvent(*state, sAIEvaluateEvent);

		if (evaluateEvent == nullptr)
		{
			continue;
		}

		statesToEvaluate.emplace_back(StateToEvaluate{
			storage, *state, *evaluateEvent, evaluateEvent->GetProperties().Has(Props::sIsEventStaticTag)
		});
	}

	const auto& enemyAIControllerView = world.GetRegistry().View<EnemyAiControllerComponent>();

	for (auto [entity, currentAIController] : enemyAIControllerView.each())
	{
		float bestScore = std::numeric_limits<float>::lowest();
		const MetaType* bestType = nullptr;

		for (const StateToEvaluate& state : statesToEvaluate)
		{
			if (!state.mStorage.get().contains(entity))
			{
				continue;
			}

			float score{};
			FuncResult evalResult;

			if (state.mAreEventsStatic)
			{
				evalResult = state.mEvaluate.get().InvokeUncheckedUnpackedWithRVO(&score, world, entity);
			}
			else
			{
				// const_cast is fine since we are assigning it to a const MetaAny
				const MetaAny component{state.mType, const_cast<void*>(state.mStorage.get().value(entity)), false};
				evalResult = state.mEvaluate.get().InvokeUncheckedUnpackedWithRVO(&score, component, world, entity);
			}

			if (evalResult.HasError())
			{
				LOG(LogAI, Error, "Error occured while evaluating state {} - {}", state.mType.get().GetName(),
				    evalResult.Error());
				continue;
			}

			if (score > bestScore)
			{
				bestScore = score;
				bestType = &state.mType.get();
			}
		}

		if (currentAIController.mCurrentState != bestType)
		{
			CallTransitionEvent(sAIStateExitEvent, currentAIController.mCurrentState, world, entity);
			currentAIController.mCurrentState = bestType;
			CallTransitionEvent(sAIStateEnterEvent, currentAIController.mCurrentState, world, entity);
		}
	}
}

template <typename EventT>
void Engine::AIEvaluateSystem::CallTransitionEvent(const EventT& event, const MetaType* type, World& world,
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
		MetaAny component{*type, storage->value(owner), false};
		boundEvent->InvokeUncheckedUnpacked(component, world, owner);
	}
}

Engine::MetaType Engine::AIEvaluateSystem::Reflect()
{
	return MetaType{MetaType::T<AIEvaluateSystem>{}, "AIEvaluateSystem", MetaType::Base<System>{}};
}
