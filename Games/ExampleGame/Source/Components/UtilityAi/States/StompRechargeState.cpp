#include "Precomp.h"
#include "Components/UtililtyAi/States/StompRechargeState.h"

#include "Components/TransformComponent.h"
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
#include "Components/UtililtyAi/States/StompExecutionState.h"
#include "Components/UtililtyAi/States/StompStartState.h"


void Game::StompRechargeState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mStompRechargeAnimation, 0.0f);
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "A PhysicsBody2D component is needed to run the DashRecharge State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = { 0,0 };

	mCurrentRechargeTimer += dt;


	auto* dashingState = world.GetRegistry().TryGet<StompExecutionState>(owner);

	if (dashingState == nullptr)
	{
		LOG(LogAI, Warning, "An DashingState is needed to run the DashRecharge State!");
		return;
	}

	auto* chargeDashingState = world.GetRegistry().TryGet<StompStartState>(owner);

	if (chargeDashingState == nullptr)
	{
		LOG(LogAI, Warning, "A ChargeDashState is needed to run the DashRecharge State!");
		return;
	}

	if (mCurrentRechargeTimer >= mMaxRechargeTime)
	{
		chargeDashingState->mCurrentStompStartTimer = 0;
		dashingState->mCurrentStompTimer = 0;
	}
}

float Game::StompRechargeState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto* dashingState = world.GetRegistry().TryGet<StompExecutionState>(owner);

	if (dashingState == nullptr)
	{
		LOG(LogAI, Warning, "A DashingState is needed to run the DashRecharge State!");
		return 0;
	}

	if (dashingState->IsStompCharged() && mCurrentRechargeTimer < mMaxRechargeTime)
	{
		return 1;
	}

	return 0;
}

void Game::StompRechargeState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		LOG(LogAI, Warning, "An NavMeshAgent component is needed to run the DashRechargeState State!");
		return;
	}

	navMeshAgent->ClearTarget(world);
}

CE::MetaType Game::StompRechargeState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<StompRechargeState>{}, "StompRechargeState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&StompRechargeState::mMaxRechargeTime, "mMaxRechargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &StompRechargeState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &StompRechargeState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &StompRechargeState::OnAIStateEnterEvent);

	type.AddField(&StompRechargeState::mStompRechargeAnimation, "mStompRechargeAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<StompRechargeState>(type);
	return type;
}
