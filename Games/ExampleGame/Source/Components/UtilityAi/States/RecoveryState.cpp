#include "Precomp.h"
#include "Components/UtililtyAi/States/RecoveryState.h"

#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Assets/Animation/Animation.h"
#include "Components/PlayerComponent.h"
#include "Components/TransformComponent.h"

void Game::RecoveryState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mDashRechargeAnimation, 0.0f);
	}
	
	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "A PhysicsBody2D component is needed to run the DashRecharge State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = {};

	mRechargeCooldown.mAmountOfTimePassed += dt;

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

float Game::RecoveryState::OnAiEvaluate(const CE::World&, entt::entity) const
{
	if (mRechargeCooldown.mAmountOfTimePassed != 0.0f && mRechargeCooldown.mAmountOfTimePassed < mRechargeCooldown.mCooldown)
	{
		return 1.0f;
	}

	return 0;
}

void Game::RecoveryState::OnAiStateEnterEvent(CE::World&, entt::entity)
{
	mRechargeCooldown.mCooldown = mMaxRechargeTime;
	mRechargeCooldown.mAmountOfTimePassed = 0.00000000000001f;
}

void Game::RecoveryState::OnAiStateExitEvent(CE::World&, entt::entity)
{
	mRechargeCooldown.mAmountOfTimePassed = 0.0f;
}

CE::MetaType Game::RecoveryState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<RecoveryState>{}, "RecoveryState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&RecoveryState::mMaxRechargeTime, "mMaxRechargeTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &RecoveryState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &RecoveryState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &RecoveryState::OnAiStateEnterEvent);
	BindEvent(type, CE::sAIStateExitEvent, &RecoveryState::OnAiStateExitEvent);

	type.AddField(&RecoveryState::mDashRechargeAnimation, "mDashRechargeAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<RecoveryState>(type);
	return type;
}
