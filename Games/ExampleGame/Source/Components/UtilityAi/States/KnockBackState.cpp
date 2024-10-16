#include "Precomp.h"
#include "Components/UtililtyAi/States/KnockBackState.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Systems/AbilitySystem.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Animation/Animation.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpStompState.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"
#include "Utilities/AiFunctionality.h"

void Game::KnockBackState::OnTick(CE::World& world, entt::entity owner, float)
{
	auto* enemyAiController = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if (enemyAiController == nullptr)
	{
		return;
	}

	if (enemyAiController->mCurrentState == nullptr)
	{
		LOG(LogAI, Warning, "No current States in the enemyAIController!");
		return;
	}

	if (CE::MakeTypeId<KnockBackState>() != enemyAiController->mCurrentState->GetTypeId()
		&& mKnockBackSpeed > 0.f
		&& !mUltimateKnockBack)
	{
		mKnockBackSpeed = 0.f;
	}
}

void Game::KnockBackState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Knock Back State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	mKnockBackSpeed *= 1.f / (1.f + (dt * mFriction));
	physicsBody2DComponent->mLinearVelocity = -mKnockbackDirection * mKnockBackSpeed;
}

float Game::KnockBackState::OnAiEvaluate(const CE::World&, entt::entity) const
{
	if (mKnockBackSpeed > mMinKnockBackSpeed)
	{
		if (mUltimateKnockBack)
		{
			return 1.0f;
		}

		return 0.825f;
	}

	return 0;
}

void Game::KnockBackState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner)
{
	AIFunctionality::AnimationInAi(world, owner, mKnockBackAnimation, false);

	const entt::entity playerId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (playerId == entt::null)
	{
		LOG(LogAI, Warning, "Knock Back State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	if (playerId != entt::null)
	{
		const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(playerId);

		if (transformComponent == nullptr)
		{
			LOG(LogAI, Warning, "An Transform component on the player entity is needed to run the Dashing State!");
			return;
		}

		const auto* ownerTransformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

		if (ownerTransformComponent == nullptr)
		{
			LOG(LogAI, Warning, "A transform component is needed to run the Dashing State!");
			return;
		}

		const glm::vec2 targetT = transformComponent->GetWorldPosition2D();

		const glm::vec2 ownerT = ownerTransformComponent->GetWorldPosition2D();

		if (targetT != ownerT)
		{
			mKnockbackDirection = glm::normalize(targetT - ownerT);
		}
	}

	CE::SwarmingAgentTag::StopMovingToTarget(world, owner);
}

void Game::KnockBackState::OnAiStateExitEvent(CE::World& world, const entt::entity owner)
{
	mUltimateKnockBack = false;
	CE::SwarmingAgentTag::StartMovingToTarget(world, owner);
}

void Game::KnockBackState::AddKnockback(const float knockbackValue, const bool ultimateKnockback)
{
	mKnockBackSpeed = std::max(mKnockBackSpeed, knockbackValue);

	mUltimateKnockBack |= ultimateKnockback;
}

void Game::KnockBackState::OnAnimationFinish(CE::World& world, entt::entity owner)
{
	const auto* enemy = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if (enemy == nullptr)
	{
		return;
	}

	if (CE::MakeTypeId<KnockBackState>() == enemy->mCurrentState->GetTypeId())
	{
		auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

		if (animationRootComponent == nullptr)
		{
			LOG(LogAI, Warning, "A animation root component is needed to run the KnockingBack State!");
			return;
		}

		animationRootComponent->SwitchAnimation(world.GetRegistry(), mKnockBackAnimation, animationRootComponent->mCurrentAnimation->mDuration - 1.0f, 0.0f, 0.0f  );
	}
}

CE::MetaType Game::KnockBackState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<KnockBackState>{}, "KnockBackState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&KnockBackState::mMinKnockBackSpeed, "Minimum Knockback Speed Before It Stops").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&KnockBackState::mFriction, "Friction").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&KnockBackState::mKnockBackAnimation, "Knockback Animation").GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&KnockBackState::mKnockBackSpeed, "Knockback Speed").GetProperties().Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsEditorReadOnlyTag);
	type.AddField(&KnockBackState::mKnockbackDirection, "Knockback Direction").GetProperties().Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsEditorReadOnlyTag);
	type.AddField(&KnockBackState::mUltimateKnockBack, "Ultimate KnockBack").GetProperties().Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsEditorReadOnlyTag);

	BindEvent(type, CE::sOnAITick, &KnockBackState::OnAiTick);
	BindEvent(type, CE::sOnTick, &KnockBackState::OnTick);
	BindEvent(type, CE::sOnAIEvaluate, &KnockBackState::OnAiEvaluate);
	BindEvent(type, CE::sOnAIStateEnter, &KnockBackState::OnAiStateEnterEvent);
	BindEvent(type, CE::sOnAIStateExit, &KnockBackState::OnAiStateExitEvent);
	BindEvent(type, CE::sOnAnimationFinish, &KnockBackState::OnAnimationFinish);


	CE::ReflectComponentType<KnockBackState>(type);
	return type;
}
