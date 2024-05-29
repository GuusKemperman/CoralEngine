#include "Precomp.h"
#include "Components/UtililtyAi/States/StompState.h"

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
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Components/UtililtyAi/States/ChargeUpStompState.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"

void Game::StompState::OnAiTick(CE::World&, const entt::entity, const float dt)
{
	mStompCooldown.mAmountOfTimePassed += dt;
}

float Game::StompState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto* chargingUpState = world.GetRegistry().TryGet<ChargeUpStompState>(owner);

	if (chargingUpState == nullptr)
	{
		LOG(LogAI, Warning, "A ChargeUpStompState is needed to run the Stomp State!");
		return 0;
	}

	auto* enemyAiController = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if (enemyAiController == nullptr)
	{
		LOG(LogAI, Warning, "A enemyAiController is needed to run the Stomp State!");
		return 0;
	}

	if (enemyAiController->mCurrentState == nullptr)
	{
		return 0;
	}

	if ((CE::MakeTypeId<ChargeUpStompState>() == enemyAiController->mCurrentState->GetTypeId() && chargingUpState->IsCharged())
		|| (CE::MakeTypeId<StompState>() == enemyAiController->mCurrentState->GetTypeId() && mStompCooldown.mAmountOfTimePassed < mStompCooldown.mCooldown))
	{
		return 0.9f;
	}

	return 0;
}

void Game::StompState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
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

	mStompCooldown.mCooldown = mMaxStompTime;
	mStompCooldown.mAmountOfTimePassed = 0.0f;
}

// OnAIStateExit()
// // Check if it has the recharge state
// If we have one, set the timer to X seconds

bool Game::StompState::IsStompCharged() const
{
	if (mStompCooldown.mAmountOfTimePassed >= mStompCooldown.mCooldown)
	{
		return true;
	}

	return false;
}

std::pair<float, entt::entity> Game::StompState::GetBestScoreAndTarget(const CE::World& world,
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

CE::MetaType Game::StompState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<StompState>{}, "StompState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&StompState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&StompState::mMaxStompTime, "mMaxStompTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &StompState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &StompState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &StompState::OnAIStateEnterEvent);

	type.AddField(&StompState::mStompAnimation, "mStompAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<StompState>(type);
	return type;
}
