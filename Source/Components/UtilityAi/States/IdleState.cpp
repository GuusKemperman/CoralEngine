#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/UtililtyAi/EnemyAiControllerComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::IdleState::OnAiTick(World& world, entt::entity owner, float)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	if (navMeshAgent->GetTargetPosition().has_value())
	{
		navMeshAgent->StopNavMesh();
	}
}

float Engine::IdleState::OnAiEvaluate(const World&, entt::entity)
{
	return 1.0f;
}

Engine::MetaType Engine::IdleState::Reflect()
{
	auto type = MetaType{MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(Props::sIsScriptableTag);

	BindEvent(type, sAITickEvent, &IdleState::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &IdleState::OnAiEvaluate);

	ReflectComponentType<IdleState>(type);
	return type;
}
