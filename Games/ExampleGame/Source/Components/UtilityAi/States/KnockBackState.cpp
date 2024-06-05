#include "Precomp.h"
#include "Components/UtililtyAi/States/KnockBackState.h"

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
#include "Components/UtililtyAi/States/ChasingState.h"
#include "Components/UtililtyAi/States/DashingState.h"
#include "Components/UtililtyAi/States/ChargeUpStompState.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"


void Game::KnockBackState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	mCurrentKnockBackCountDownTimer -= dt;

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mKnockBackAnimation, 0.0f);
	}
	else
	{
		LOG(LogAI, Warning, "An animationRoot component is needed to run the KnockBack State!");
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "An PhysicsBody2D component is needed to run the Dashing State!");
		return;
	}

	const float ratio = mCurrentKnockBackCountDownTimer / mMaxKnockBackTime;

	physicsBody2DComponent->mLinearVelocity = -mDashDirection * ratio * mKnockBackSpeed;
}

float Game::KnockBackState::OnAiEvaluate(const CE::World& world, entt::entity owner)
{
	//auto [score, entity] = GetBestScoreAndTarget(world, owner);

	auto* rechargeDashState = world.GetRegistry().TryGet<ChargeUpDashState>(owner);

	if (rechargeDashState != nullptr)
	{
		if (rechargeDashState->mChargeCooldown.mAmountOfTimePassed != 0.f)
		{
			return 0;
		}
	}

	auto* dashingState = world.GetRegistry().TryGet<DashingState>(owner);

	if (dashingState != nullptr)
	{
		if (dashingState->mDashCooldown.mAmountOfTimePassed != 0.f)
		{
			return 0;
		}
	}

	if (mCurrentKnockBackCountDownTimer > 0 || (mJustGotHit))
	{
		mJustGotHit = false;
 		return 1;
	}
	mJustGotHit = false;

	return 0;
}

void Game::KnockBackState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	const auto* target = world.GetRegistry().TryGet<Game::ChasingState>(owner);

	mJustGotHit = false;

	if (target == nullptr)
	{
		LOG(LogAI, Warning, "An ChasingState is needed to run the Dashing State!");
		return;
	}

	const entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An NavMeshTargetTag on the player entity is needed to run the Dashing State!");
		return;
	}

	mTargetEntity = entityId;

	if (mTargetEntity != entt::null)
	{
		const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(mTargetEntity);

		if (transformComponent == nullptr)
		{
			LOG(LogAI, Warning, "An Transform component on the player entity is needed to run the Dashing State!");
			return;
		}

		const auto* ownerTransformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

		if (ownerTransformComponent == nullptr)
		{
			LOG(LogAI, Warning, "A transform component is needed to run the Dashing State!");
			return;
		}

		const auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

		if (physicsBody2DComponent == nullptr)
		{
			LOG(LogAI, Warning, "An PhysicsBody2D component is needed to run the Dashing State!");
			return;
		}

		const glm::vec2 targetT = transformComponent->GetWorldPosition2D();

		const glm::vec2 ownerT = ownerTransformComponent->GetWorldPosition2D();

		mDashDirection = glm::normalize(targetT - ownerT);
	}

	auto* recoverState = world.GetRegistry().TryGet<ChargeUpDashState>(owner);

	if (recoverState == nullptr)
	{
		LOG(LogAI, Warning, "An DashRechargeState is needed to run the Dashing State!");
		return;
	}

	recoverState->mChargeCooldown.mAmountOfTimePassed = 0;
}

bool Game::KnockBackState::IsDashCharged() const
{
	return mCurrentKnockBackCountDownTimer <= 0;
}

void Game::KnockBackState::ResetKnockBackTime()
{
	mCurrentKnockBackCountDownTimer = mMaxKnockBackTime;
}

CE::MetaType Game::KnockBackState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<KnockBackState>{}, "KnockBackState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&KnockBackState::mKnockBackSpeed, "mKnockBackSpeed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&KnockBackState::mMaxKnockBackTime, "mMaxKnockBackTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &KnockBackState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &KnockBackState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &KnockBackState::OnAIStateEnterEvent);

	type.AddField(&KnockBackState::mKnockBackAnimation, "mKnockBackAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&KnockBackState::mJustGotHit, "mJustGotHit").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddFunc(&KnockBackState::ResetKnockBackTime, "ResetKnockBackTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<KnockBackState>(type);
	return type;
}
