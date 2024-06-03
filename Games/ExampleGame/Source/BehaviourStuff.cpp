#include "Precomp.h"
#include "BehaviourStuff.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Systems/AbilitySystem.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Components/Pathfinding/SwarmingTargetComponent.h"

float Game::GetBestScoreBasedOnDetection(const CE::World& world, entt::entity owner, float radius)
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	const entt::entity entityId = world.GetRegistry().View<CE::SwarmingTargetComponent>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with a SwarmAgentTag is needed to run the Charging Up State!");
		return 0.0f;
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "TransformComponent is needed to run the Charging Up State!");
		return 0.0f;
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "The player entity needs a TransformComponent is needed to run the Charging Up State!");
		return 0.0f;
	}

	const float distance = glm::distance(transformComponent->GetWorldPosition(), targetComponent->GetWorldPosition());

	float score = 0.0f;

	if (distance < radius)
	{
		score = 1 / distance;
		score += 1 / radius;
	}

	if (highestScore < score)
	{
		highestScore = score;
	}

	return highestScore;
}

void Game::ExecuteEnemyAbility(CE::World& world, entt::entity owner)
{
	const auto characterData = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);
	if (characterData == nullptr)
	{
		LOG(LogAI, Warning, "A character component is needed to run the Attacking State!");
		return;
	}

	const auto abilities = world.GetRegistry().TryGet<CE::AbilitiesOnCharacterComponent>(owner);
	if (abilities == nullptr)
	{
		LOG(LogAI, Warning, "A AbilitiesOnCharacter component is needed to run the Attacking State!");
		return;
	}

	if (abilities->mAbilitiesToInput.empty())
	{
		return;
	}

	CE::AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);
}
