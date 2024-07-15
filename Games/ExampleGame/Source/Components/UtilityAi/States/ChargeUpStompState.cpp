#include "Precomp.h"
#include "Components/UtililtyAi/States/ChargeUpStompState.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Systems/AbilitySystem.h"
#include "Utilities/AiFunctionality.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"

void Game::ChargeUpStompState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Charge Up Stomp State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	mCurrentTime += dt;

	AIFunctionality::FaceThePlayer(world, owner);
}

float Game::ChargeUpStompState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{

	if (mCurrentTime != 0.0f)
	{
		return 0.85f;
	}

	const auto score = AIFunctionality::GetBestScoreBasedOnDetection(world, owner, mRadius);

	return score;
}

void Game::ChargeUpStompState::OnAiStateExitEvent(CE::World& world, entt::entity)
{
	world.GetRegistry().Destroy(mSpawnedVFX, true);

	mCurrentTime = 0.0f;
}

void Game::ChargeUpStompState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	AIFunctionality::AnimationInAi(world, owner, mChargingAnimation, false);

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

bool Game::ChargeUpStompState::IsCharged() const
{
	if (mCurrentTime >= mMaxChargeTime)
	{
		return true;
	}

	return false;
}

CE::MetaType Game::ChargeUpStompState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ChargeUpStompState>{}, "ChargeUpStompState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ChargeUpStompState::mRadius, "Detection Radius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpStompState::mMaxChargeTime, "Max Charge Time").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpStompState::mCurrentTime, "Current Time").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag);
	type.AddField(&ChargeUpStompState::mChargingAnimation, "Charging Animation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpStompState::mVFX, "VFX").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpStompState::mSpawnedVFX, "Spawned VFX").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag);

	BindEvent(type, CE::sOnAITick, &ChargeUpStompState::OnAiTick);
	BindEvent(type, CE::sOnAIEvaluate, &ChargeUpStompState::OnAiEvaluate);
	BindEvent(type, CE::sOnAIStateEnter, &ChargeUpStompState::OnAiStateEnterEvent);
	BindEvent(type, CE::sOnAIStateExit, &ChargeUpStompState::OnAiStateExitEvent);

	CE::ReflectComponentType<ChargeUpStompState>(type);
	return type;
}
