#include "Precomp.h"
#include "Components/UtililtyAi/States/DashingState.h"

#include "Utilities/AiFunctionality.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeUpDashState.h"
#include "Components/UtililtyAi/States/RecoveryState.h"
#include "Assets/Animation/Animation.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"


void Game::DashingState::OnAiTick(CE::World& world, const entt::entity owner, const float dt)
{
	mDashCooldown.mAmountOfTimePassed += dt;

	if (mDashCooldown.mAmountOfTimePassed >= mDashCooldown.mCooldown)
	{
		const auto recoveryState = world.GetRegistry().TryGet<Game::RecoveryState>(owner);

		if (recoveryState == nullptr)
		{
			LOG(LogAI, Warning, "Dash State - enemy {} does not have a RecoveryState Component.", entt::to_integral(owner));
		}
		else
		{
			recoveryState->mRechargeCooldown.mAmountOfTimePassed = 0.0f;
		}
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "Dash State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
		return;
	}

	physicsBody2DComponent->mLinearVelocity = mDashDirection * mSpeedDash;
 }

float Game::DashingState::OnAiEvaluate(const CE::World& world, const entt::entity owner) const
{
	auto* chargingUpState = world.GetRegistry().TryGet<ChargeUpDashState>(owner);

	if (chargingUpState == nullptr)
	{
		LOG(LogAI, Warning, "Dash State - enemy {} does not have a ChargeUpDash State.", entt::to_integral(owner));
		return 0;
	}

	auto* enemyAiController = world.GetRegistry().TryGet<CE::EnemyAiControllerComponent>(owner);

	if (enemyAiController == nullptr)
	{
		LOG(LogAI, Warning, "Dash State - enemy {} does not have a EnemyAiController Component.", entt::to_integral(owner));
		return 0;
	}

	if (enemyAiController->mCurrentState == nullptr)
	{
		return 0;
	}

	if ((CE::MakeTypeId<ChargeUpDashState>() == enemyAiController->mCurrentState->GetTypeId() && chargingUpState->IsCharged())
		|| (CE::MakeTypeId<DashingState>() == enemyAiController->mCurrentState->GetTypeId() && mDashCooldown.mAmountOfTimePassed < mDashCooldown.mCooldown))
	{
		return 0.9f;
	}

	return 0;
}

void Game::DashingState::OnAiStateEnterEvent(CE::World& world, const entt::entity owner)
{
	Game::AnimationInAi(world, owner, mDashingAnimation, false);

	const entt::entity entityId = world.GetRegistry().View<CE::PlayerComponent>().front();

	if (entityId == entt::null)
	{
		return;
	}

	mTargetEntity = entityId;

	if (mTargetEntity != entt::null)
	{
		const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(mTargetEntity);

		if (transformComponent == nullptr)
		{
			LOG(LogAI, Warning, "Dash State - player {} does not have a Transform Component.", entt::to_integral(mTargetEntity));
			return;
		}

		const auto* ownerTransformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

		if (ownerTransformComponent == nullptr)
		{
			LOG(LogAI, Warning, "Dash State - enemy {} does not have a Transform Component.", entt::to_integral(owner));
			return;
		}

		const auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

		if (physicsBody2DComponent == nullptr)
		{
			LOG(LogAI, Warning, "Dash State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(owner));
			return;
		}

		const glm::vec2 targetT = transformComponent->GetWorldPosition2D();

		const glm::vec2 ownerT = ownerTransformComponent->GetWorldPosition2D();

		mDashDirection = glm::normalize(targetT - ownerT);
	}

	mDashCooldown.mCooldown = mMaxDashTime;
	mDashCooldown.mAmountOfTimePassed = 0.0f;
}

bool Game::DashingState::IsDashCharged() const
{
	if (mDashCooldown.mAmountOfTimePassed >= mDashCooldown.mCooldown)
	{
		return true;
	}

	return false;
}

CE::MetaType Game::DashingState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DashingState>{}, "DashingState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);
	
	type.AddField(&DashingState::mSpeedDash, "mSpeedDash").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DashingState::mMaxDashTime, "mMaxDashTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DashingState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DashingState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DashingState::OnAiStateEnterEvent);

	type.AddField(&DashingState::mDashingAnimation, "mDashingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DashingState>(type);
	return type;
}