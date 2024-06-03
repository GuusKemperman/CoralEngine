#include "Precomp.h"
#include "Components/UtililtyAi/States/StompState.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Systems/AbilitySystem.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Components/UtililtyAi/States/ChargeUpStompState.h"
#include "Components/UtililtyAi/States/RecoveryState.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"

void Game::StompState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	mStompCooldown.mAmountOfTimePassed += dt;

	if (mStompCooldown.mAmountOfTimePassed >= mStompCooldown.mCooldown)
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

	const auto characterData = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);
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
	CE::AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mStompAnimation, 0.0f);
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	const entt::entity playerId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (playerId == entt::null)
	{
		return;
	}

	const auto playerTransform = world.GetRegistry().TryGet<CE::TransformComponent>(playerId);
	if (playerTransform == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - player {} does not have a Transform Component.", entt::to_integral(playerId));
		return;
	}

	const auto enemyTransform = world.GetRegistry().TryGet<CE::TransformComponent>(owner);
	if (enemyTransform == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
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

float Game::StompState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto* chargingUpState = world.GetRegistry().TryGet<ChargeUpStompState>(owner);

	if (chargingUpState == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a ChargeUpState State.", entt::to_integral(owner));
		return 0;
	}

	auto* enemyAiController = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if (enemyAiController == nullptr)
	{
		LOG(LogAI, Warning, "Stomp State - enemy {} does not have a EnemyAiController Component.", entt::to_integral(owner));
		return 0;
	}

	if (enemyAiController->mCurrentState == nullptr)
	{
		return 0;
	}

	if ((CE::MakeTypeId<ChargeUpStompState>() == enemyAiController->mCurrentState->GetTypeId() && chargingUpState->IsCharged())
		|| (CE::MakeTypeId<StompState>() == enemyAiController->mCurrentState->GetTypeId() && mStompCooldown.mAmountOfTimePassed < mStompCooldown.mCooldown))
	{
		return 0.9f;
	}

	return 0;
}

void Game::StompState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mStompAnimation, 0.0f);
	}

	mStompCooldown.mCooldown = mMaxStompTime;
	mStompCooldown.mAmountOfTimePassed = 0.0f;
}

bool Game::StompState::IsStompCharged() const
{
	if (mStompCooldown.mAmountOfTimePassed >= mStompCooldown.mCooldown)
	{
		return true;
	}

	return false;
}

CE::MetaType Game::StompState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<StompState>{}, "StompState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&StompState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&StompState::mMaxStompTime, "mMaxStompTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &StompState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &StompState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &StompState::OnAIStateEnterEvent);

	type.AddField(&StompState::mStompAnimation, "mStompAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<StompState>(type);
	return type;
}
