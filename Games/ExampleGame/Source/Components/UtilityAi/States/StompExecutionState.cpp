#include "Precomp.h"
#include "Components/UtililtyAi/States/StompExecutionState.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Systems/AbilitySystem.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargingUpState.h"

void Game::StompExecutionState::OnAiTick(CE::World&, const entt::entity, const float dt)
{
	mCurrentStompTimer += dt;
}

float Game::StompExecutionState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	// if (!ChargingUpState::IsCharged()) { return 0.0f; } 

	auto* chargingUpState = world.GetRegistry().TryGet<ChargingUpState>(owner);

	if (chargingUpState == nullptr)
	{
		LOG(LogAI, Warning, "A ChargingUpState is needed to run the Dashing State!");
		return 0;
	}

	if (chargingUpState->IsCharged() && mCurrentStompTimer < mMaxStompTime)
	{
		return 0.9f;
	}

	return 0;
}

void Game::StompExecutionState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		LOG(LogAI, Warning, "NavMeshAgentComponent is needed to run the Charge Dash State!");
		return;
	}

	navMeshAgent->ClearTarget(world);

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mStompAnimation, 0.0f);
	}
}

// OnAIStateExit()
// // Check if it has the recharge state
// If we have one, set the timer to X seconds

bool Game::StompExecutionState::IsStompCharged() const
{
	return (mCurrentStompTimer >= mMaxStompTime);
}

std::pair<float, entt::entity> Game::StompExecutionState::GetBestScoreAndTarget(const CE::World& world,
	entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with a NavMeshTargetTag is needed to run the Charge Dash State!");
		return { 0.0f, entt::null };
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "TransformComponent is needed to run the Charge Dash State!");
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "The player entity needs a TransformComponent is needed to run the Charge Dash State!");
		return { 0.0f, entt::null };
	}

	const float distance = glm::distance(transformComponent->GetWorldPosition(), targetComponent->GetWorldPosition());

	float score = 0.0f;

	if (distance < mRadius)
	{
		score = 1 / distance;
		score += 1 / mRadius;
	}

	if (highestScore < score)
	{
		highestScore = score;
	}

	return { highestScore, entityId };
}

CE::MetaType Game::StompExecutionState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<StompExecutionState>{}, "StompExecutionState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&StompExecutionState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&StompExecutionState::mMaxStompTime, "mMaxStompTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &StompExecutionState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &StompExecutionState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &StompExecutionState::OnAIStateEnterEvent);

	type.AddField(&StompExecutionState::mStompAnimation, "mStompAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<StompExecutionState>(type);
	return type;
}
