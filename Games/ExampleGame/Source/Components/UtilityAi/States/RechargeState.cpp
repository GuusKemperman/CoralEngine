#include "Precomp.h"
#include "Components/UtililtyAi/States/RechargeState.h"

#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Components/UtililtyAi/States/DashingState.h"
#include "Assets/Animation/Animation.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Components/UtililtyAi/States/StompState.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"

void Game::RechargeState::OnAiTick(CE::World& world, entt::entity owner, float dt)
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

	mRechargeCooldown.IsReady(dt);
}

float Game::RechargeState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto* enemyAiController = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if (enemyAiController == nullptr)
	{
		LOG(LogAI, Warning, "A EnemyAiController is needed to run the Stomp State!");
		return 0;
	}

	if (enemyAiController->mCurrentState == nullptr)
	{
		return 0;
	}

	if (CE::MakeTypeId<DashingState>() == enemyAiController->mCurrentState->GetTypeId())
	{
		auto* dashingState = world.GetRegistry().TryGet<DashingState>(owner);

		if (dashingState == nullptr)
		{
			LOG(LogAI, Warning, "A DashingState is needed to run the Recharge State!");
			return 0;
		}

		if (!dashingState->IsDashCharged())
		{
			return 0.0f;
		}

		if (mRechargeCooldown.mAmountOfTimePassed < mRechargeCooldown.mCooldown)
		{
			return 1.0f;
		}
	}
	else if (CE::MakeTypeId<StompState>() == enemyAiController->mCurrentState->GetTypeId())
	{
		auto* stompState = world.GetRegistry().TryGet<StompState>(owner);

		if (stompState == nullptr)
		{
			LOG(LogAI, Warning, "A StompState is needed to run the Recharge State!");
			return 0;
		}

		if (!stompState->IsStompCharged())
		{
			return 0.0f;
		}

		if (mRechargeCooldown.mAmountOfTimePassed < mRechargeCooldown.mCooldown)
		{
			return 1.0f;
		}
	}

	return 0;
}

void Game::RechargeState::OnAiStateEnterEvent(CE::World&, entt::entity)
{
	mRechargeCooldown.mCooldown = mMaxRechargeTime;
	mRechargeCooldown.mAmountOfTimePassed = 0.0f;
}

CE::MetaType Game::RechargeState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<RechargeState>{}, "RechargeState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&RechargeState::mMaxRechargeTime, "mMaxRechargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &RechargeState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &RechargeState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &RechargeState::OnAiStateEnterEvent);

	type.AddField(&RechargeState::mDashRechargeAnimation, "mDashRechargeAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<RechargeState>(type);
	return type;
}
