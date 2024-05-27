#include "Precomp.h"
#include "Components/UtililtyAi/States/DashRechargeState.h"

#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeDashState.h"
#include "Components/UtililtyAi/States/DashingState.h"
#include "Assets/Animation/Animation.h"

void Game::DashRechargeState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mDashRechargeAnimation, 0.0f);
	}
	
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "A PhysicsBody2D component is needed to run the DashRecharge State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {0,0};

	mCurrentRechargeTimer += dt;


	auto* dashingState = world.GetRegistry().TryGet<DashingState>(owner);

	if (dashingState == nullptr)
	{
		LOG(LogAI, Warning, "An DashingState is needed to run the DashRecharge State!");
		return;
	}

	auto* chargeDashingState = world.GetRegistry().TryGet<ChargeDashState>(owner);

	if (chargeDashingState == nullptr)
	{
		LOG(LogAI, Warning, "A ChargeDashState is needed to run the DashRecharge State!");
		return;
	}

	if (mCurrentRechargeTimer >= mMaxRechargeTime)
	{
		chargeDashingState->mCurrentChargeTimer = 0;
		dashingState->mCurrentDashTimer = 0;
	}
}

float Game::DashRechargeState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto* dashingState = world.GetRegistry().TryGet<DashingState>(owner);

	if (dashingState == nullptr)
	{
		LOG(LogAI, Warning, "A DashingState is needed to run the DashRecharge State!");
		return 0;
	}

	if (dashingState->IsDashCharged() && mCurrentRechargeTimer < mMaxRechargeTime)
	{
		return 1;
	}

	return 0;
}

CE::MetaType Game::DashRechargeState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DashRechargeState>{}, "DashRechargeState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&DashRechargeState::mMaxRechargeTime, "mMaxRechargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DashRechargeState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DashRechargeState::OnAiEvaluate);

	type.AddField(&DashRechargeState::mDashRechargeAnimation, "mDashRechargeAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DashRechargeState>(type);
	return type;
}
