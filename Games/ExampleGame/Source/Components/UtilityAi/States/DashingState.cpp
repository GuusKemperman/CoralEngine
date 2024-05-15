#include "Precomp.h"
#include "Components/UtililtyAi/States/DashingState.h"

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
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtililtyAi/States/ChargeDashState.h"
#include "Components/UtililtyAi/States/ChasingState.h"
#include "Components/UtililtyAi/States/DashRechargeState.h"


void Game::DashingState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	mCurrentDashTimer += dt;

	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mDashingAnimation, 0.0f);
	}

	auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

	if (physicsBody2DComponent == nullptr)
	{
		return;
	}

	physicsBody2DComponent->mLinearVelocity = mDashDirection * mSpeedDash;
 }

float Game::DashingState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	//auto [score, entity] = GetBestScoreAndTarget(world, owner);

	auto* chargeDashState = world.GetRegistry().TryGet<ChargeDashState>(owner);

	if (chargeDashState == nullptr)
	{
		return 0;
	}

	if (chargeDashState->IsDashCharged() && mCurrentDashTimer < mMaxDashTime)
	{
		return 0.9f;
	}

	return 0;
}

void Game::DashingState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	const auto* target = world.GetRegistry().TryGet<Game::ChasingState>(owner);

	if (target == nullptr)
	{
		return;
	}

	const entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

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
			return;
		}

		const auto* ownerTransformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

		if (ownerTransformComponent == nullptr)
		{
			return;
		}

		const auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

		if (physicsBody2DComponent == nullptr)
		{
			return;
		}

		const glm::vec2 targetT = transformComponent->GetWorldPosition2D();

		const glm::vec2 ownerT = ownerTransformComponent->GetWorldPosition2D();

		mDashDirection = glm::normalize(targetT - ownerT);
	}

	auto* recoverState = world.GetRegistry().TryGet<DashRechargeState>(owner);

	if (recoverState == nullptr)
	{
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