#include "Precomp.h"
#include "Components/UtililtyAi/States/ChasingState.h"

#include "Components/TransformComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"

void Game::ChasingState::OnAiTick(CE::World& world, entt::entity owner, float)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mChasingAnimation, 0.0f);
	}

	auto [score, targetEntity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = targetEntity;

	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		LOG(LogAI, Warning, "NavMeshAgentComponent is needed to run the Chasing State!");
		return;
	}

	if (mTargetEntity != entt::null)
	{
		navMeshAgent->SetTargetEntity(mTargetEntity);
	}
}

float Game::ChasingState::OnAiEvaluate(const CE::World& world, entt::entity owner)
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = entity;
	return score;
}

std::pair<float, entt::entity> Game::ChasingState::GetBestScoreAndTarget(
	const CE::World& world, entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with the NavMeshTargetTag is needed to run the Chasing State!");
		return { 0.0f, entt::null };
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "A transform component is needed to run the Chasing State!");
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "The entity with the NavMeshTargetTag needs a TranformComponent is needed to run the Chasing State!");
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

void Game::ChasingState::DebugRender(CE::World& world, entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "A transform component is needed to run the Chasing State!");
		return;
	}

	DrawDebugCircle(
		world, CE::DebugCategory::Gameplay,
		transformComponent->GetWorldPosition(),
		mRadius, {0.f, 0.f, 1.f, 1.f});
}

CE::MetaType Game::ChasingState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<ChasingState>{}, "ChasingState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ChasingState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ChasingState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ChasingState::OnAiEvaluate);

	type.AddField(&ChasingState::mChasingAnimation, "mChasingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChasingState>(type);
	return type;
}
