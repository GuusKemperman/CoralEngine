#include "Precomp.h"
#include "Components/UtililtyAi/States/DashingState.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/AnimationRootComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargingUpState.h"
#include "Components/UtililtyAi/States/DashRechargeState.h"
#include "Assets/Animation/Animation.h"


void Game::DashingState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	// If NotReadyYet
	// Show outline, increase NotReadyYet timer
	// else
	// Dash forward
	//

	// OnAIEvaulate
	// If dashing timer finished && ChargeUpS timer finished return 0.0f

	// OnAIExit
	// Set rest timer on RestComponent to N seconds


	mCurrentDashTimer += dt;

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mDashingAnimation, 0.0f);
	}
	else
	{
		LOG(LogAI, Warning, "An animationRoot component is needed to run the Dashing State!");
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		LOG(LogAI, Warning, "An PhysicsBody2D component is needed to run the Dashing State!");
		return;
	}

	physicsBody2DComponent->mLinearVelocity = mDashDirection * mSpeedDash;
 }

float Game::DashingState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto* chargingUpState = world.GetRegistry().TryGet<ChargingUpState>(owner);

	if (chargingUpState == nullptr)
	{
		LOG(LogAI, Warning, "A ChargingUpState is needed to run the Dashing State!");
		return 0;
	}

	if (chargingUpState->IsCharged() && mCurrentDashTimer < mMaxDashTime)
	{
		return 0.9f;
	}

	return 0;
}

void Game::DashingState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
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
			LOG(LogAI, Warning, "An Transform component on the player entity is needed to run the Dashing State!");
			return;
		}

		const auto* ownerTransformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

		if (ownerTransformComponent == nullptr)
		{
			LOG(LogAI, Warning, "A transform component is needed to run the Dashing State!");
			return;
		}

		const auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

		if (physicsBody2DComponent == nullptr)
		{
			LOG(LogAI, Warning, "An PhysicsBody2D component is needed to run the Dashing State!");
			return;
		}

		const glm::vec2 targetT = transformComponent->GetWorldPosition2D();

		const glm::vec2 ownerT = ownerTransformComponent->GetWorldPosition2D();

		mDashDirection = glm::normalize(targetT - ownerT);
	}

	auto* recoverState = world.GetRegistry().TryGet<DashRechargeState>(owner);

	if (recoverState == nullptr)
	{
		LOG(LogAI, Warning, "An DashRechargeState is needed to run the Dashing State!");
		return;
	}

	recoverState->mCurrentRechargeTimer = 0;
}

bool Game::DashingState::IsDashCharged() const
{
	return mCurrentDashTimer >= mMaxDashTime;
}

CE::MetaType Game::DashingState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DashingState>{}, "DashingState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);
	
	type.AddField(&DashingState::mSpeedDash, "mSpeedDash").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DashingState::mMaxDashTime, "mMaxDashTime").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DashingState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DashingState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DashingState::OnAIStateEnterEvent);

	type.AddField(&DashingState::mDashingAnimation, "mDashingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DashingState>(type);
	return type;
}