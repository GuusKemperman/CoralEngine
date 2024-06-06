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

void Game::ChasingState::OnAiStateEnter(CE::World& world, const entt::entity owner) const
{
	Game::AnimationInAi(world, owner, mChasingAnimation);

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

	type.AddField(&ChasingState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAIStateEnterEvent, &ChasingState::OnAiStateEnter);
	BindEvent(type, CE::sAIStateExitEvent, &ChasingState::OnAiStateExit);
	BindEvent(type, CE::sAIEvaluateEvent, &ChasingState::OnAiEvaluate);

	type.AddField(&ChasingState::mChasingAnimation, "mChasingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChasingState>(type);
	return type;
}
