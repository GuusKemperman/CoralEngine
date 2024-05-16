#include "Precomp.h"
#include "Components/UtililtyAi/States/ChargeDashState.h"

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
#include "Systems/AbilitySystem.h"


void Game::ChargeDashState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mChargingAnimation, 0.0f);
	}

	mCurrentChargeTimer += dt;
}

float Game::ChargeDashState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);

	if (mCurrentChargeTimer > 0 && mCurrentChargeTimer < mMaxChargeTime)
	{
		return 0.9f;
	}

	if (mCurrentChargeTimer < mMaxChargeTime)
	{
		return score;
	}

	return 0;
}

void Game::ChargeDashState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		LOG(LogSeverity, Warning, "NavMeshAgentComponent is needed to run the Charge Dash State!");
		return;
	}

	navMeshAgent->StopNavMesh();
}

bool Game::ChargeDashState::IsDashCharged() const
{
	return (mCurrentChargeTimer >= mMaxChargeTime);
}

std::pair<float, entt::entity> Game::ChargeDashState::GetBestScoreAndTarget(const CE::World& world,
                                                                            entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogSeverity, Warning, "An entity with a NavMeshTargetTag is needed to run the Charge Dash State!");
		return { 0.0f, entt::null };
	}

	if (transformComponent == nullptr)
	{
		LOG(LogSeverity, Warning, "TransformComponent is needed to run the Charge Dash State!");
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogSeverity, Warning, "The player entity needs a TransformComponent is needed to run the Charge Dash State!");
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

CE::MetaType Game::ChargeDashState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ChargeDashState>{}, "ChargeDashState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ChargeDashState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeDashState::mMaxChargeTime, "mMaxChargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ChargeDashState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ChargeDashState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &ChargeDashState::OnAIStateEnterEvent);

	type.AddField(&ChargeDashState::mChargingAnimation, "mChargingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChargeDashState>(type);
	return type;
}
