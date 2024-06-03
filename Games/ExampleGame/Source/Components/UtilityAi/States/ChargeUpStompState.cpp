#include "Precomp.h"
#include "Components/UtililtyAi/States/ChargeUpStompState.h"

#include "Components/TransformComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Systems/AbilitySystem.h"


void Game::ChargeUpStompState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mChargingAnimation, 0.0f);
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "An PhysicsBody2D component is needed to run the ChargingUp State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	mChargeCooldown.mAmountOfTimePassed += dt;

	const entt::entity playerId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (playerId == entt::null)
	{
		return;
	}

	const auto playerTransform = world.GetRegistry().TryGet<CE::TransformComponent>(playerId);
	if (playerTransform == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a AbilitiesOnCharacter Component.", entt::to_integral(owner));
		return;
	}

	const auto enemyTransform = world.GetRegistry().TryGet<CE::TransformComponent>(owner);
	if (enemyTransform == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a AbilitiesOnCharacter Component.", entt::to_integral(owner));
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

float Game::ChargeUpStompState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{

	if (mChargeCooldown.mAmountOfTimePassed != 0.0f)
	{
		return 0.8f;
	}

	auto [score, entity] = GetBestScoreAndTarget(world, owner);

	return score;
}

void Game::ChargeUpStompState::OnAiStateExitEvent(CE::World&, entt::entity)
{
	mChargeCooldown.mAmountOfTimePassed = 0.0f;
}

void Game::ChargeUpStompState::OnAiStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		LOG(LogAI, Warning, "NavMeshAgentComponent is needed to run the Charging Up State!");
		return;
	}

	navMeshAgent->ClearTarget(world);

	mChargeCooldown.mCooldown = mMaxChargeTime;
	mChargeCooldown.mAmountOfTimePassed = 0.0f;
}

bool Game::ChargeUpStompState::IsCharged() const
{
	if (mChargeCooldown.mAmountOfTimePassed >= mChargeCooldown.mCooldown)
	{
		return true;
	}
	
	return false;
}

std::pair<float, entt::entity> Game::ChargeUpStompState::GetBestScoreAndTarget(const CE::World& world,
	entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with a NavMeshTargetTag is needed to run the Charging Up State!");
		return { 0.0f, entt::null };
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "TransformComponent is needed to run the Charging Up State!");
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "The player entity needs a TransformComponent is needed to run the Charging Up State!");
		return { 0.0f, entt::null };
	}

	const float distance = glm::distance(transformComponent->GetWorldPosition(), targetComponent->GetWorldPosition());

	float score = 0.0f;

	if (distance < mRadius)
	{
		score = 1 / distance;
		score += 1 / mRadius;
	}

	if (highestScore < score)
	{
		highestScore = score;
	}

	return { highestScore, entityId };
}

CE::MetaType Game::ChargeUpStompState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ChargeUpStompState>{}, "ChargeUpStompState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ChargeUpStompState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ChargeUpStompState::mMaxChargeTime, "mMaxChargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ChargeUpStompState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ChargeUpStompState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &ChargeUpStompState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAIStateExitEvent, &ChargeUpStompState::OnAiStateExitEvent);

	type.AddField(&ChargeUpStompState::mChargingAnimation, "mChargingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ChargeUpStompState>(type);
	return type;
}
