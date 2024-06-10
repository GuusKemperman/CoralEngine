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

	if (mShootCooldown.mAmountOfTimePassed >= mShootCooldown.mCooldown)
	{
		const auto rechargeState = world.GetRegistry().TryGet<Game::RecoveryState>(owner);

		if (rechargeState == nullptr)
		{
			LOG(LogAI, Warning, "Stomp State - enemy {} does not have a RecoveryState Component.", entt::to_integral(owner));
		}
		else
		{
			rechargeState->mRechargeCooldown.mAmountOfTimePassed = 0.1f;
		}
	}

	Game::ExecuteEnemyAbility(world, owner);

	Game::AnimationInAi(world, owner, mStompAnimation);

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

	Game::AnimationInAi(world, owner, mStompAnimation);

	mShootCooldown.mCooldown = mMaxStompTime;
	mShootCooldown.mAmountOfTimePassed = 0.0f;
}

void Game::ShootingBowState::OnAiStateExitEvent(CE::World& , entt::entity)
{
	mShootCooldown.mAmountOfTimePassed = 0.0f;
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

	type.AddField(&ShootingBowState::mStompAnimation, "mStompAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ShootingBowState>(type);
	return type;
}
