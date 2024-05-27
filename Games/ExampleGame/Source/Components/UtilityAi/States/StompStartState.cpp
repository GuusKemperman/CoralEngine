#include "Precomp.h"
#include "Components/UtililtyAi/States/StompStartState.h"

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

void Game::StompStartState::OnAiTick(CE::World& world, entt::entity owner, float)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mStompStartAnimation, 0.0f);
	}

	auto [score, targetEntity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = targetEntity;

	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		return;
	}

	if (mTargetEntity != entt::null)
	{
		navMeshAgent->SetTargetEntity(mTargetEntity);
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "A PhysicsBody2D component is needed to run the DashRecharge State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = { 0,0 };
}

float Game::StompStartState::OnAiEvaluate(const CE::World& world, entt::entity owner)
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = entity;
	return score;
}

void Game::StompStartState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		LOG(LogAI, Warning, "A NavMeshAgent component is needed to run the Attacking State!");
		return;
	}

	navMeshAgent->ClearTarget(world);
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "A PhysicsBody2D component is needed to run the DashRecharge State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = { 0,0 };
}

std::pair<float, entt::entity> Game::StompStartState::GetBestScoreAndTarget(const CE::World& world,
	entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with a NavMeshTarget component is needed to run the Attacking State!");
		return { 0.0f, entt::null };
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "A transform component is needed to run the Attacking State!");
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "A transform component on the player entity is needed to run the Attacking State!");
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

CE::MetaType Game::StompStartState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<StompStartState>{}, "StompStartState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&StompStartState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &StompStartState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &StompStartState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &StompStartState::OnAIStateEnterEvent);

	type.AddField(&StompStartState::mStompStartAnimation, "mStompStartAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<StompStartState>(type);
	return type;
}
