#include "Precomp.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"

#include "Components/TransformComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Systems/AbilitySystem.h"


void Game::ChargeUpDashState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
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

	mCurrentChargeTimer += dt;
}

float Game::ChargeUpDashState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	if (mChargeCooldown.mAmountOfTimePassed != 0.0f)
	{
		return 0.8f;
	}

	auto [score, entity] = GetBestScoreAndTarget(world, owner);

	return score;
}

void Game::ChargeUpDashState::OnAiStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		LOG(LogAI, Warning, "NavMeshAgentComponent is needed to run the Charging Up State!");
		return;
	}

	navMeshAgent->ClearTarget(world);

	mChargeCooldown.mCooldown = mMaxChargeTime;
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

std::pair<float, entt::entity> Game::ChargeUpDashState::GetBestScoreAndTarget(const CE::World& world,
                                                                            entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with a NavMeshTargetTag is needed to run the Charging Up State!");
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

CE::MetaType Game::ChargeUpDashState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ChargeUpDashState>{}, "ChargeUpDashState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ChargeUpDashState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpDashState::mMaxChargeTime, "mMaxChargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ChargeUpDashState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ChargeUpDashState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &ChargeUpDashState::OnAiStateEnterEvent);

	type.AddField(&ChargeUpDashState::mChargingAnimation, "mChargingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChargeUpDashState>(type);
	return type;
}
