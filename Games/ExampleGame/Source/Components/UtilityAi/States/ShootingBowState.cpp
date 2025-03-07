#include "Precomp.h"
#include "Components/UtililtyAi/States/ShootingBowState.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Systems/AbilitySystem.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Components/UtililtyAi/States/RecoveryState.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"
#include "Utilities/AiFunctionality.h"

void Game::ShootingBowState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "A PhysicsBody2D component is needed to run the DashRecharge State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	mShootCooldown.mAmountOfTimePassed += dt;

	AIFunctionality::ExecuteEnemyAbility(world, owner);

	AIFunctionality::FaceThePlayer(world, owner);
}

float Game::ShootingBowState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{
	if (mShootCooldown.mAmountOfTimePassed != 0.0f)
	{
		return 0.8f;
	}

	return  AIFunctionality::GetBestScoreBasedOnDetection(world, owner, mRadius);
}

void Game::ShootingBowState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	AIFunctionality::AnimationInAi(world, owner, mShootingAnimation, false);

	mShootCooldown.mCooldown = mMaxShootTime;
	mShootCooldown.mAmountOfTimePassed = 0.0f;
}

void Game::ShootingBowState::OnAiStateExitEvent(CE::World& , entt::entity)
{
	mShootCooldown.mAmountOfTimePassed = 0.0f;
}

void Game::ShootingBowState::OnFinishAnimationEvent(CE::World& world, entt::entity owner)
{
	const auto* enemyAiController = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if (enemyAiController == nullptr)
	{
		return;
	}

	if (enemyAiController->mCurrentState == nullptr)
	{
		return;
	}

	if (CE::MakeTypeId<ShootingBowState>() == enemyAiController->mCurrentState->GetTypeId())
	{
		const auto recoveryState = world.GetRegistry().TryGet<Game::RecoveryState>(owner);

		if (recoveryState == nullptr)
		{
			LOG(LogAI, Warning, "Stomp State - enemy {} does not have a RecoveryState Component.", entt::to_integral(owner));
		}
		else
		{
			recoveryState->mRechargeCooldown.mAmountOfTimePassed = 0.1f;
		}
	}
}

bool Game::ShootingBowState::IsShootingCharged() const
{
	if (mShootCooldown.mAmountOfTimePassed >= mShootCooldown.mCooldown)
	{
		return true;
	}

	return false;
}

CE::MetaType Game::ShootingBowState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ShootingBowState>{}, "ShootingBowState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ShootingBowState::mRadius, "Detection Radius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ShootingBowState::mMaxShootTime, "Max Shoot Time").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sOnAITick, &ShootingBowState::OnAiTick);
	BindEvent(type, CE::sOnAIEvaluate, &ShootingBowState::OnAiEvaluate);
	BindEvent(type, CE::sOnAIStateEnter, &ShootingBowState::OnAiStateEnterEvent);
	BindEvent(type, CE::sOnAIStateExit, &ShootingBowState::OnAiStateExitEvent);
	BindEvent(type, CE::sOnAnimationFinish, &ShootingBowState::OnFinishAnimationEvent);

	type.AddField(&ShootingBowState::mShootingAnimation, "Shooting Animation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ShootingBowState>(type);
	return type;
}
