#include "Precomp.h"
#include "AiFunctionality.h"

#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Systems/AbilitySystem.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Components/Pathfinding/SwarmingTargetComponent.h"

float Game::GetBestScoreBasedOnDetection(const CE::World& world, const entt::entity owner, const float radius)
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

void Game::ExecuteEnemyAbility(CE::World& world, const entt::entity owner)
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

void Game::AnimationInAi(CE::World& world, const entt::entity owner, const CE::AssetHandle<CE::Animation>& animation)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), animation, 0.0f);
	}
	else
	{
		LOG(LogAI, Warning, "Enemy {} does not have a AnimationRoot Component.", entt::to_integral(owner));
	}
}

void Game::FaceThePlayer(CE::World& world, const entt::entity owner)
{
	const entt::entity playerId = world.GetRegistry().View<CE::SwarmingTargetComponent>().front();

	if (playerId == entt::null)
	{
		return;
	}

	const auto playerTransform = world.GetRegistry().TryGet<CE::TransformComponent>(playerId);
	if (playerTransform == nullptr)
	{
		LOG(LogAI, Warning, "Charge Up Stomp State - player {} does not have a Transform Component.", entt::to_integral(playerId));
		return;
	}

	const auto enemyTransform = world.GetRegistry().TryGet<CE::TransformComponent>(owner);
	if (enemyTransform == nullptr)
	{
		LOG(LogAI, Warning, "Charge Up Stomp State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
		return;
	}

	const glm::vec2 playerPosition2D = playerTransform->GetWorldPosition2D();
	const glm::vec2 enemyPosition2D = enemyTransform->GetWorldPosition2D();

	if (playerPosition2D != enemyPosition2D)
	{
		const glm::vec2 direction = glm::normalize(playerPosition2D - enemyPosition2D);

		enemyTransform->SetWorldOrientation(CE::Math::Direction2DToXZQuatOrientation(direction));
	}
}
