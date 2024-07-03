#include "Precomp.h"
#include "Components/UtililtyAi/States/RunningAwayState.h"

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
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Utilities/Random.h"

void Game::RunningAwayState::OnAiStateEnter(CE::World& world, const entt::entity owner)
{
	mStartedEscaping = true;
	
	AIFunctionality::AnimationInAi(world, owner, mChasingAnimation, true);

	CE::SwarmingAgentTag::StartMovingToTarget(world, owner);

	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "Running Away State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}

	for (CE::TransformComponent& child : transformComponent->GetChildren())
	{
		child.SetLocalPosition({ child.GetLocalPosition().x, 0, child.GetLocalPosition().z });
	}

	const auto physicsBody = world.GetRegistry().HasComponent<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody)
	{
		LOG(LogAI, Warning, "Running Away State - enemy {} already has a Physics Component.", entt::to_integral(owner));
		return;
	}

	CE::PhysicsBody2DComponent& physicsBody2D = world.GetRegistry().AddComponent<CE::PhysicsBody2DComponent>(owner);

	physicsBody2D.mRules = CE::CollisionPresets::sCharacter.mRules;
}

void Game::RunningAwayState::OnAiStateExit(CE::World& world, const entt::entity owner)
{
	mStartedEscaping = false;

	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);
}

void Game::RunningAwayState::OnAiTick(CE::World& world, const entt::entity owner, float) const
{
	if (AIFunctionality::GetBestScoreBasedOnDetection(world, owner, mDestroyRadius) < 0.01f) 
	{
		world.GetRegistry().Destroy(owner, true);
	}
}

float Game::RunningAwayState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{
	if (mStartedEscaping)
	{
		return 0.8f;
	}

	const auto score = AIFunctionality::GetBestScoreBasedOnDetection(world, owner, mRadius);
	return score;
}

void Game::RunningAwayState::OnBeginPlay(CE::World& world, const entt::entity owner) const
{
	auto* characterComponent = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);

	if (characterComponent == nullptr)
	{
		LOG(LogAI, Warning, "Running Away State - enemy {} does not have a Character Component.", entt::to_integral(owner));
		return;
	}

	characterComponent->mCurrentMovementSpeed = -(characterComponent->mCurrentMovementSpeed) + CE::Random::Range(mLowerSpeedRange, mUpperSpeedRange);

	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "Running Away State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}

	for (CE::TransformComponent& child : transformComponent->GetChildren())
	{
		child.SetLocalPosition({child.GetLocalPosition().x, mStartYAxis, child.GetLocalPosition().z});
	}
}

void Game::RunningAwayState::DebugRender(CE::World& world, const entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "Running Away State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}

	DrawDebugCircle(
		world, CE::DebugCategory::Gameplay,
		transformComponent->GetWorldPosition(),
		mRadius, { 0.f, 0.f, 1.f, 1.f });
}

CE::MetaType Game::RunningAwayState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<RunningAwayState>{}, "RunningAwayState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&RunningAwayState::mRadius, "Detection Radius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&RunningAwayState::mDestroyRadius, "Destroy Radius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&RunningAwayState::mLowerSpeedRange, "The Lowest Random Speed Range").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&RunningAwayState::mUpperSpeedRange, "The Highest Random Speed Range").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAIStateEnterEvent, &RunningAwayState::OnAiStateEnter);
	BindEvent(type, CE::sAIStateExitEvent, &RunningAwayState::OnAiStateExit);
	BindEvent(type, CE::sAIEvaluateEvent, &RunningAwayState::OnAiEvaluate);
	BindEvent(type, CE::sBeginPlayEvent, &RunningAwayState::OnBeginPlay);
	BindEvent(type, CE::sAITickEvent, &RunningAwayState::OnAiTick);

	type.AddField(&RunningAwayState::mStartYAxis, "Start Y Position").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&RunningAwayState::mChasingAnimation, "Running Away Animation").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&RunningAwayState::mRunAnimationSpeed, "Run Animation Speed").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<RunningAwayState>(type);
	return type;
}
