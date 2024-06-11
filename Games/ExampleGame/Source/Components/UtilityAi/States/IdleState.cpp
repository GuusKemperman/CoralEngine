#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Utilities/AiFunctionality.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"

float Game::IdleState::OnAiEvaluate(const CE::World&, entt::entity)
{
	return 0.01f;
}

void Game::IdleState::OnAiStateEnterEvent(CE::World& world, entt::entity owner) const
{
	Game::AnimationInAi(world, owner, mIdleAnimation, true);

	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->ClearTarget(world);
}

CE::MetaType Game::IdleState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);
	
	BindEvent(type, CE::sAIEvaluateEvent, &IdleState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &IdleState::OnAiStateEnterEvent);

	type.AddField(&IdleState::mIdleAnimation, "Idle Animation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<IdleState>(type);
	return type;
}
