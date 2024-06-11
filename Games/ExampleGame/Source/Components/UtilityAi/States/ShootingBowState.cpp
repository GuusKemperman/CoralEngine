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

	Game::ExecuteEnemyAbility(world, owner);

	Game::FaceThePlayer(world, owner);
}

float Game::ShootingBowState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{
	if (mShootCooldown.mAmountOfTimePassed != 0.0f)
	{
		return 0.8f;
	}

	return  Game::GetBestScoreBasedOnDetection(world, owner, mRadius);
}

void Game::ShootingBowState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	Game::AnimationInAi(world, owner, mShootingAnimation, false);

	mShootCooldown.mCooldown = mMaxStompTime;
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
		LOG(LogAI, Warning, "Dash State - enemy {} does not have a EnemyAiController Component.", entt::to_integral(owner));
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
			auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

			if (animationRootComponent != nullptr)
			{
				animationRootComponent->SwitchAnimation(world.GetRegistry(), recoveryState->mRecoveryAnimation, 0.0f, 1, 0.0f);
			}
			else
			{
				LOG(LogAI, Warning, "Enemy {} does not have a AnimationRoot Component.", entt::to_integral(owner));
			}

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

	type.AddField(&ShootingBowState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ShootingBowState::mMaxStompTime, "mMaxStompTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ShootingBowState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ShootingBowState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &ShootingBowState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAIStateExitEvent, &ShootingBowState::OnAiStateExitEvent);
	BindEvent(type, CE::sAnimationFinishEvent, &ShootingBowState::OnFinishAnimationEvent);

	type.AddField(&ShootingBowState::mShootingAnimation, "mShootingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ShootingBowState>(type);
	return type;
}
