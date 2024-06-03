#include "Precomp.h"
#include "BehaviourStuff.h"

#include "Components/TransformComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "World/Registry.h"
#include "World/World.h"

float Game::GetBestScoreBasedOnDetection(const CE::World& world, entt::entity owner, float radius)
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	const entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with a NavMeshTargetTag is needed to run the Charging Up State!");
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
