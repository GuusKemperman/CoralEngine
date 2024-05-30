#include "Precomp.h"
#include "Components/UtililtyAi/States/ShootingBowState.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Systems/AbilitySystem.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Components/UtililtyAi/States/RecoveryState.h"

void Game::ShootingBowState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "A PhysicsBody2D component is needed to run the DashRecharge State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	mShootCooldown.mAmountOfTimePassed += dt;

	if (mShootCooldown.mAmountOfTimePassed >= mShootCooldown.mCooldown)
	{
		const auto rechargeState = world.GetRegistry().TryGet<Game::RecoveryState>(owner);

		if (rechargeState == nullptr)
		{
			LOG(LogAI, Warning, "Stomp State - enemy {} does not have a RecoveryState Component.", entt::to_integral(owner));
		}
		else
		{
			rechargeState->mRechargeCooldown.mAmountOfTimePassed = 0.1f;
		}
	}

	/*const auto characterData = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);
	if (characterData == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a Character Component.", entt::to_integral(owner));
		return;
	}

	const auto abilities = world.GetRegistry().TryGet<CE::AbilitiesOnCharacterComponent>(owner);
	if (abilities == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a AbilitiesOnCharacter Component.", entt::to_integral(owner));
		return;
	}

	if (abilities->mAbilitiesToInput.empty())
	{
		return;
	}
	CE::AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);*/

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mStompAnimation, 0.0f);
	}

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

	const glm::vec3 direction = glm::normalize(playerTransform->GetWorldPosition() - enemyTransform->GetWorldPosition());

	enemyTransform->SetWorldOrientation(direction);
	enemyTransform->SetWorldForward(direction);
}

float Game::ShootingBowState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	if (mShootCooldown.mAmountOfTimePassed != 0.0f)
	{
		return 0.8f;
	}

	auto [score, entity] = GetBestScoreAndTarget(world, owner);

	return score;
}

void Game::ShootingBowState::OnAiStateEnterEvent(CE::World& world, entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mStompAnimation, 0.0f);
	}

	mShootCooldown.mCooldown = mMaxStompTime;
	mShootCooldown.mAmountOfTimePassed = 0.0f;
}

void Game::ShootingBowState::OnAiStateExitEvent(CE::World& , entt::entity)
{
	mShootCooldown.mAmountOfTimePassed = 0.0f;
}

bool Game::ShootingBowState::IsShootingCharged() const
{
	if (mShootCooldown.mAmountOfTimePassed >= mShootCooldown.mCooldown)
	{
		return true;
	}

	return false;
}

std::pair<float, entt::entity> Game::ShootingBowState::GetBestScoreAndTarget(const CE::World& world, entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with a NavMeshTargetTag is needed to run the Charge Dash State!");
		return { 0.0f, entt::null };
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "TransformComponent is needed to run the Charge Dash State!");
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "The player entity needs a TransformComponent is needed to run the Charge Dash State!");
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

CE::MetaType Game::ShootingBowState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<ShootingBowState>{}, "ShootingBowState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ShootingBowState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&ShootingBowState::mMaxStompTime, "mMaxStompTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ShootingBowState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ShootingBowState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &ShootingBowState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAIStateExitEvent, &ShootingBowState::OnAiStateExitEvent);

	type.AddField(&ShootingBowState::mStompAnimation, "mStompAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<ShootingBowState>(type);
	return type;
}
