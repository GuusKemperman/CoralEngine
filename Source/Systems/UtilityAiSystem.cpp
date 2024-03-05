#include "Precomp.h"
#include "Systems/UtilityAiSystem.h"

#include "Components/UtililtyAi/EnemyAiControllerComponent.h"
#include "World/Registry.h"
#include "World/World.h"

void Engine::UtilityAiSystem::Update(World& world, float dt)
{
	const auto& enemyAIControllerView = world.GetRegistry().View<EnemyAiControllerComponent>();

	for (auto [enemyAIControllerID, currentAIController] : enemyAIControllerView.each())
	{
		currentAIController.UpdateState();
	}
}

Engine::MetaType Engine::UtilityAiSystem::Reflect()
{
	return MetaType{MetaType::T<UtilityAiSystem>{}, "UtilityAiSystem", MetaType::Base<System>{}};
}
