#include "Precomp.h"
#include "Components/UtililtyAi/States/DashRechargeState.h"

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
#include "Components/UtililtyAi/States/DashingState.h"


void Game::DashRechargeState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	animationRootComponent->SwitchAnimation(world.GetRegistry(), mDashingAnimation, 0.0f);

	if (mTargetEntity != entt::null)
	{
		auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

		if (physicsBody2DComponent == nullptr) { return; }

		physicsBody2DComponent->mLinearVelocity = {0,0};
	}

	mCurrentRechargeTimer += dt;
}

float Game::DashRechargeState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	//auto [score, entity] = GetBestScoreAndTarget(world, owner);

	auto* dashingState = world.GetRegistry().TryGet<DashingState>(owner);

	if (dashingState == nullptr) { return 0; }

	if (dashingState->IsDashCharged() && mCurrentRechargeTimer >= mMaxRechargeTime)
	{
		return 0.9f;
	}

	return 0;
}

void Game::DashRechargeState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->StopNavMesh();
}

std::pair<float, entt::entity> Game::DashRechargeState::GetBestScoreAndTarget(const CE::World& world,
	entt::entity owner) const
{
	const auto targetsView = world.GetRegistry().View<CE::NavMeshTargetTag, CE::TransformComponent>();
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;
	entt::entity entityId = entt::null;

	for (auto [targetId, targetTransform] : targetsView.each())
	{
		const float distance = glm::distance(transformComponent->GetWorldPosition(),
			targetTransform.GetWorldPosition());

		float score = 0.0f;

		if (distance < mRadius)
		{
			score = 1 / distance;
			score += 1 / mRadius;
		}

		if (highestScore < score)
		{
			highestScore = score;
			entityId = targetId;
		}
	}

	return { highestScore, entityId };
}

CE::MetaType Game::DashRechargeState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DashRechargeState>{}, "DashRechargeState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&DashRechargeState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DashRechargeState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DashRechargeState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DashRechargeState::OnAIStateEnterEvent);

	type.AddField(&DashRechargeState::mDashingAnimation, "mDashingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DashRechargeState>(type);
	return type;
}
