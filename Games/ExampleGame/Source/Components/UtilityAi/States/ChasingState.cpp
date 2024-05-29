#include "Precomp.h"
#include "Components/UtililtyAi/States/ChasingState.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Assets/Animation/Animation.h"

void Game::ChasingState::OnAIStateEnter(CE::World& world, entt::entity owner)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mChasingAnimation, 0.0f);
	}
	
	CE::SwarmingAgentTag::StartMovingToTarget(world, owner);
}

void Game::ChasingState::OnAIStateExit(CE::World& world, entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);
}

float Game::ChasingState::OnAIEvaluate(const CE::World& world, entt::entity owner)
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);
	return score;
}

std::pair<float, entt::entity> Game::ChasingState::GetBestScoreAndTarget(
	const CE::World& world, entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (entityId == entt::null)
	{
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

	float score = 0.1f;

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

	BindEvent(type, CE::sAIStateEnterEvent, &ChasingState::OnAIStateEnter);
	BindEvent(type, CE::sAIStateExitEvent, &ChasingState::OnAIStateExit);
	BindEvent(type, CE::sAIEvaluateEvent, &ChasingState::OnAIEvaluate);

	type.AddField(&ChasingState::mChasingAnimation, "mChasingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChasingState>(type);
	return type;
}
