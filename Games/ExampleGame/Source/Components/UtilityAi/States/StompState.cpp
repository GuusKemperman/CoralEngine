#include "Precomp.h"
#include "Components/UtililtyAi/States/StompState.h"

#include "Utilities/AiFunctionality.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Systems/AbilitySystem.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/PlayerComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Components/UtililtyAi/States/ChargeUpStompState.h"
#include "Components/UtililtyAi/States/RecoveryState.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"

void Game::StompState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	mCurrentTime += dt;

	if (mCurrentTime >= mMaxStompTime)
	{
		const auto recoveryState = world.GetRegistry().TryGet<Game::RecoveryState>(owner);

		if (recoveryState == nullptr)
		{
			LOG(LogAI, Warning, "Stomp State - enemy {} does not have a RecoveryState Component.", entt::to_integral(owner));
		}
		else
		{
			recoveryState->mRechargeCooldown.mAmountOfTimePassed = 0.0f;
		}
	}

	AIFunctionality::ExecuteEnemyAbility(world, owner);

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	AIFunctionality::FaceThePlayer(world, owner);
}

float Game::StompState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{
	auto* chargingUpState = world.GetRegistry().TryGet<ChargeUpStompState>(owner);

	if (chargingUpState == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a ChargeUpState State.", entt::to_integral(owner));
		return 0;
	}

	auto* enemyAiController = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if (enemyAiController == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a EnemyAiController Component.", entt::to_integral(owner));
		return 0;
	}

	if (enemyAiController->mCurrentState == nullptr)
	{
		return 0;
	}

	if ((CE::MakeTypeId<ChargeUpStompState>() == enemyAiController->mCurrentState->GetTypeId() && chargingUpState->IsCharged())
		|| (CE::MakeTypeId<StompState>() == enemyAiController->mCurrentState->GetTypeId() && mCurrentTime < mMaxStompTime))
	{
		return 0.9f;
	}

	return 0;
}

void Game::StompState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	AIFunctionality::AnimationInAi(world, owner, mStompAnimation, false);

	auto* transform = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transform == nullptr)
	{
		LOG(LogAI, Warning, "Charge Up Stomp State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	if (mVFX != nullptr) {
		mSpawnedVFX = world.GetRegistry().CreateFromPrefab(*mVFX, entt::null, nullptr, nullptr, nullptr, transform);
	}

	mCurrentTime = 0.0f;
}

bool Game::StompState::IsStompCharged() const
{
	if (mCurrentTime >= mMaxStompTime)
	{
		return true;
	}

	return false;
}

CE::MetaType Game::StompState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<StompState>{}, "StompState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&StompState::mRadius, "Detection Radius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&StompState::mMaxStompTime, "Max Stomp Time").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&StompState::mCurrentTime, "Current Time").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag);
	type.AddField(&StompState::mStompAnimation, "Stomp Animation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&StompState::mVFX, "VFX").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&StompState::mSpawnedVFX, "Spawned VFX").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag);

	BindEvent(type, CE::sAITickEvent, &StompState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &StompState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &StompState::OnAiStateEnterEvent);

	CE::ReflectComponentType<StompState>(type);
	return type;
}
