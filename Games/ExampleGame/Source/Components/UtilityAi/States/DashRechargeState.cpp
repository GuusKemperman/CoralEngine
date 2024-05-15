#include "Precomp.h"
#include "Components/UtililtyAi/States/DashRechargeState.h"

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
#include "Components/UtililtyAi/States/ChargeDashState.h"
#include "Components/UtililtyAi/States/DashingState.h"


void Game::DashRechargeState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent == nullptr) { return; }

	animationRootComponent->SwitchAnimation(world.GetRegistry(), mDashRechargeAnimation, 0.0f);

	if (mTargetEntity != entt::null)
	{
		auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

		if (physicsBody2DComponent == nullptr) { return; }

		physicsBody2DComponent->mLinearVelocity = {0,0};
	}

	mCurrentRechargeTimer += dt;

	auto* dashingState = world.GetRegistry().TryGet<DashingState>(owner);

	if (dashingState == nullptr) { return; }

	auto* chargeDashingState = world.GetRegistry().TryGet<ChargeDashState>(owner);

	if (chargeDashingState == nullptr) { return; }

	if (mCurrentRechargeTimer >= mMaxRechargeTime)
	{
		chargeDashingState->mCurrentChargeTimer = 0;
		dashingState->mCurrentDashTimer = 0;
	}
}

float Game::DashRechargeState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto* dashingState = world.GetRegistry().TryGet<DashingState>(owner);

	if (dashingState == nullptr) { return 0; }

	if (dashingState->IsDashCharged() && mCurrentRechargeTimer < mMaxRechargeTime)
	{
		return 1;
	}

	return 0;
}

void Game::DashRechargeState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->StopNavMesh();
}

CE::MetaType Game::DashRechargeState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DashRechargeState>{}, "DashRechargeState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&DashRechargeState::mMaxRechargeTime, "mMaxRechargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DashRechargeState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DashRechargeState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DashRechargeState::OnAIStateEnterEvent);

	type.AddField(&DashRechargeState::mDashRechargeAnimation, "mDashRechargeAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DashRechargeState>(type);
	return type;
}
