#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Utilities/AiFunctionality.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"

float Game::IdleState::OnAiEvaluate(const CE::World&, entt::entity)
{
	return 0.01f;
}

void Game::IdleState::OnAiStateEnterEvent(CE::World& world, entt::entity owner) const
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);
	AIFunctionality::AnimationInAi(world, owner, mIdleAnimation, true);

	auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "Running Away State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}

	const glm::vec3 newPosition = { transformComponent->GetWorldPosition().x, mStartYAxis, transformComponent->GetWorldPosition().z };

	transformComponent->SetWorldPosition(newPosition);
}

void Game::IdleState::OnAiStateExitEvent(CE::World& world, const entt::entity owner)
{
	auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "Running Away State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}

	const glm::vec3 newPosition = { transformComponent->GetWorldPosition().x, 0, transformComponent->GetWorldPosition().z };

	transformComponent->SetWorldPosition(newPosition);
}

CE::MetaType Game::IdleState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);
	
	BindEvent(type, CE::sAIEvaluateEvent, &IdleState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &IdleState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAIStateExitEvent, &IdleState::OnAiStateExitEvent);

	type.AddField(&IdleState::mStartYAxis, "Start Y Position").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&IdleState::mIdleAnimation, "Idle Animation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<IdleState>(type);
	return type;
}
