#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Game::IdleState::OnAiTick(Engine::World&, entt::entity, float)
{
}

float Game::IdleState::OnAiEvaluate(const Engine::World&, entt::entity)
{
	return 1.0f;
}

void Game::IdleState::OnAIStateEnterEvent(Engine::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<Engine::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->StopNavMesh();
}

Engine::MetaType Game::IdleState::Reflect()
{
	auto type = Engine::MetaType{Engine::MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(Engine::Props::sIsScriptableTag);

	BindEvent(type, Engine::sAITickEvent, &IdleState::OnAiTick);
	BindEvent(type, Engine::sAIEvaluateEvent, &IdleState::OnAiEvaluate);
	BindEvent(type, Engine::sAIStateEnterEvent, &IdleState::OnAIStateEnterEvent);

	Engine::ReflectComponentType<IdleState>(type);
	return type;
}
