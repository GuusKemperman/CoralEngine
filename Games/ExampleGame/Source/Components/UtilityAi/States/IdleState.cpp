#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Game::IdleState::OnAiTick(CE::World&, entt::entity, float)
{
}

float Game::IdleState::OnAiEvaluate(const CE::World&, entt::entity)
{
	return 1.0f;
}

void Game::IdleState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->StopNavMesh();
}

CE::MetaType Game::IdleState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &IdleState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &IdleState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &IdleState::OnAIStateEnterEvent);

	CE::ReflectComponentType<IdleState>(type);
	return type;
}
