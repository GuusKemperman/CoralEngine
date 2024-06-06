#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Utilities/AiFunctionality.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"

void Game::IdleState::OnAiTick(CE::World& world, const entt::entity owner, float) const
{
	Game::AnimationInAi(world, owner, mIdleAnimation);
}

float Game::IdleState::OnAiEvaluate(const CE::World&, entt::entity)
{
	return 0.01f;
}

void Game::IdleState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->ClearTarget(world);
}

CE::MetaType Game::IdleState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &IdleState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &IdleState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &IdleState::OnAiStateEnterEvent);

	type.AddField(&IdleState::mIdleAnimation, "mIdleAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<IdleState>(type);
	return type;
}
