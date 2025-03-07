#include "Precomp.h"
#include "Utilities/AiFunctionality.h"

#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/UtililtyAi/States/KnockBackState.h"
#include "Systems/AbilitySystem.h"
#include "Utilities/Random.h"
#include "World/Registry.h"
#include "World/World.h"

float Game::AIFunctionality::GetBestScoreBasedOnDetection(const CE::World& world, const entt::entity owner, const float radius)
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	const entt::entity entityId = world.GetRegistry().View<CE::PlayerComponent, CE::TransformComponent>().front();

	if (entityId == entt::null)
	{
		return 0.0f;
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "TransformComponent is needed to run the Charging Up State!");
		return 0.0f;
	}

	float highestScore = 0.0f;

	auto& targetComponent = world.GetRegistry().Get<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "The player entity needs a TransformComponent is needed to run the Charging Up State!");
		return 0.0f;
	}

	const float distance = glm::distance(transformComponent->GetWorldPosition(), targetComponent.GetWorldPosition());

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

void Game::AIFunctionality::ExecuteEnemyAbility(CE::World& world, const entt::entity owner)
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

void Game::AIFunctionality::AnimationInAi(CE::World& world, const entt::entity owner, const CE::AssetHandle<CE::Animation>& playAIAnimation, bool startAtRandomTime)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent == nullptr)
	{
		LOG(LogAI, Warning, "Enemy {} does not have a AnimationRoot Component.", entt::to_integral(owner));
		return;
	}

	float timeStamp = 0.0f;

	if (playAIAnimation != nullptr
		&& startAtRandomTime)
	{
		timeStamp = CE::Random::Range(0.0f, playAIAnimation->mDuration);
	}

	animationRootComponent->SwitchAnimation(world.GetRegistry(), playAIAnimation, timeStamp);
}

void Game::AIFunctionality::FaceThePlayer(CE::World& world, const entt::entity owner)
{
	const entt::entity playerId = world.GetRegistry().View<CE::PlayerComponent>().front();

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

CE::MetaType Game::AIFunctionality::Reflect()
{
	auto metaType = CE::MetaType{ CE::MetaType::T<AIFunctionality>{}, "AIFunctionality" };
	metaType.GetProperties().Add(CE::Props::sIsScriptableTag);

	metaType.AddFunc([](entt::entity enemy, float knockbackValue, bool ultimate)
		{
			CE::World* world = CE::World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			auto* knockBackState = world->GetRegistry().TryGet<KnockBackState>(enemy);
			if (knockBackState == nullptr)
			{
				LOG(LogAI, Warning, "AddKnockback - enemy {} does not have a KnockBackState.", entt::to_integral(enemy));
				return;
			}
			knockBackState->AddKnockback(knockbackValue, ultimate);

		}, "AddKnockback", CE::MetaFunc::ExplicitParams<
		entt::entity, float, bool>{}, "Enemy Entity", "Knockback Value", "Ultimate Knockback").GetProperties().Add(CE::Props::sIsScriptableTag).Set(CE::Props::sIsScriptPure, false);

		return metaType;
}
