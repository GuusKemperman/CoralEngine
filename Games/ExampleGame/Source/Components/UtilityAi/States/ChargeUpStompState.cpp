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


void Game::ChargeUpStompState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mChargingAnimation, 0.0f);
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "An PhysicsBody2D component is needed to run the ChargingUp State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	mChargeCooldown.mAmountOfTimePassed += dt;
}

float Game::ChargeUpStompState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{

	if (mChargeCooldown.mAmountOfTimePassed != 0.0f)
	{
		return 0.8f;
	}

	auto [score, entity] = GetBestScoreAndTarget(world, owner);

	return score;
}

void Game::ChargeUpStompState::OnAiStateExitEvent(CE::World&, entt::entity)
{
	mChargeCooldown.mAmountOfTimePassed = 0.0f;
}

void Game::ChargeUpStompState::OnAiStateEnterEvent(CE::World& world, entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	mChargeCooldown.mCooldown = mMaxChargeTime;
	mChargeCooldown.mAmountOfTimePassed = 0.0f;
}

bool Game::ChargeUpStompState::IsCharged() const
{
	if (mChargeCooldown.mAmountOfTimePassed >= mChargeCooldown.mCooldown)
	{
		return true;
	}
	
	return false;
}

std::pair<float, entt::entity> Game::ChargeUpStompState::GetBestScoreAndTarget(const CE::World& world,
	entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (entityId == entt::null)
	{
		return { 0.0f, entt::null };
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "TransformComponent is needed to run the Charging Up State!");
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "The player entity needs a TransformComponent is needed to run the Charging Up State!");
		return { 0.0f, entt::null };
	}

	const float distance = glm::distance(transformComponent->GetWorldPosition(), targetComponent->GetWorldPosition());

	float score = 0.0f;

	if (distance < mRadius)
	{
		score = 1 / distance;
		score += 1 / mRadius;
	}

	if (highestScore < score)
	{
		highestScore = score;
	}

	return { highestScore, entityId };
}

CE::MetaType Game::ChargeUpStompState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ChargeUpStompState>{}, "ChargeUpStompState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ChargeUpStompState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpStompState::mMaxChargeTime, "mMaxChargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ChargeUpStompState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ChargeUpStompState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &ChargeUpStompState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAIStateExitEvent, &ChargeUpStompState::OnAiStateExitEvent);

	type.AddField(&ChargeUpStompState::mChargingAnimation, "mChargingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChargeUpStompState>(type);
	return type;
}
