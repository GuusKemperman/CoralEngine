#include "Precomp.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"

#include "Utilities/AiFunctionality.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Assets/Animation/Animation.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Utilities/AiFunctionality.h"

void Game::ChargeUpDashState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "ChargeUpDash State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	mChargeCooldown.mAmountOfTimePassed += dt;

	AIFunctionality::FaceThePlayer(world, owner);
}

float Game::ChargeUpDashState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{
	if (mChargeCooldown.mAmountOfTimePassed != 0.0f)
	{
		return 0.8f;
	}

	const auto score = AIFunctionality::GetBestScoreBasedOnDetection(world, owner, mRadius);

	return score;
}

void Game::ChargeUpDashState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	AIFunctionality::AnimationInAi(world, owner, mChargingAnimation, false);

	mSpawnedVfx = world.GetRegistry().CreateFromPrefab(*mParticles, owner);

	mChargeCooldown.mCooldown = mMaxChargeTime;
	mChargeCooldown.mAmountOfTimePassed = 0.0f;
}

void Game::ChargeUpDashState::OnAiStateExitEvent(CE::World& world, entt::entity owner)
{
	auto* transform = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transform == nullptr)
	{
		LOG(LogAI, Warning, "Charge Up Stomp State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	if (mParticles != nullptr) {
		mSpawnedVfx = world.GetRegistry().CreateFromPrefab(*mParticles, entt::null, nullptr, nullptr, nullptr, transform);
	}

	mChargeCooldown.mAmountOfTimePassed = 0.0f;
}

bool Game::ChargeUpDashState::IsCharged() const
{
	if (mChargeCooldown.mAmountOfTimePassed >= mChargeCooldown.mCooldown)
	{
		return true;
	}

	return false;
}

CE::MetaType Game::ChargeUpDashState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ChargeUpDashState>{}, "ChargeUpDashState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ChargeUpDashState::mRadius, "Detection Radius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpDashState::mMaxChargeTime, "Max Charge Time").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ChargeUpDashState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ChargeUpDashState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &ChargeUpDashState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAIStateExitEvent, &ChargeUpDashState::OnAiStateExitEvent);

	type.AddField(&ChargeUpDashState::mChargingAnimation, "Charging Animation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpDashState::mParticles, "Particles").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChargeUpDashState>(type);
	return type;
}
