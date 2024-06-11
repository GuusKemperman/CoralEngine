#include "Precomp.h"
#include "Components/UtililtyAi/States/ChasingState.h"

#include "Utilities/AiFunctionality.h"
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
#include "Components/Abilities/CharacterComponent.h"
#include "Utilities/Random.h"

void Game::ChasingState::OnAiStateEnter(CE::World& world, const entt::entity owner) const
{
	Game::AnimationInAi(world, owner, mChasingAnimation, true);

	CE::SwarmingAgentTag::StartMovingToTarget(world, owner);
}

void Game::ChasingState::OnAiStateExit(CE::World& world, const entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);
}

float Game::ChasingState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{
	const auto score = GetBestScoreBasedOnDetection(world, owner, mRadius);
	return score;
}

void Game::ChasingState::OnBeginPlay(CE::World& world, const entt::entity owner) const
{
	auto* characterComponent = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);

	if (characterComponent == nullptr)
	{
		LOG(LogAI, Warning, "Chasing State - enemy {} does not have a Character Component.", entt::to_integral(owner));
		return;
	}

	characterComponent->mCurrentMovementSpeed += CE::Random::Range(mLowerSpeedRange, mUpperSpeedRange);
}

void Game::ChasingState::DebugRender(CE::World& world, const entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "Chasing State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
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

	type.AddField(&ChasingState::mRadius, "Detection Radius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChasingState::mLowerSpeedRange, "The Lowest Random Speed Range").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChasingState::mUpperSpeedRange, "The Highest Random Speed Range").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAIStateEnterEvent, &ChasingState::OnAiStateEnter);
	BindEvent(type, CE::sAIStateExitEvent, &ChasingState::OnAiStateExit);
	BindEvent(type, CE::sAIEvaluateEvent, &ChasingState::OnAiEvaluate);
	BindEvent(type, CE::sBeginPlayEvent, &ChasingState::OnBeginPlay);

	type.AddField(&ChasingState::mChasingAnimation, "Chasing Animation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChasingState>(type);
	return type;
}
