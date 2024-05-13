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


void Game::DashingState::OnAiTick(CE::World& world, entt::entity owner, float dt)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	animationRootComponent->SwitchAnimation(world.GetRegistry(), mDashingAnimation, 0.0f);

	/*auto [score, targetEntity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = targetEntity;

	const auto characterData = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);
	if (characterData == nullptr)
	{
		return;
	}

	const auto abilities = world.GetRegistry().TryGet<CE::AbilitiesOnCharacterComponent>(owner);
	if (abilities == nullptr)
	{
		return;
	}

	if (abilities->mAbilitiesToInput.empty())
	{
		return;
	}

	CE::AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);*/

	if (mTargetEntity != entt::null)
	{
		const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(mTargetEntity);

		if (transformComponent == nullptr) { return; }

		const auto* ownerTransformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

		if (ownerTransformComponent == nullptr) { return; }

		auto* physicsBody2DComponent = world.GetRegistry().TryGet<CE::PhysicsBody2DComponent>(owner);

		if (physicsBody2DComponent == nullptr) { return; }

		physicsBody2DComponent->mLinearVelocity = glm::normalize(transformComponent->GetWorldPosition() - ownerTransformComponent->GetWorldPosition()) * mSpeedDash;
	}

	mCurrentDashTimer += dt;
}

float Game::DashingState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	//auto [score, entity] = GetBestScoreAndTarget(world, owner);

	auto* chargeDashState = world.GetRegistry().TryGet<ChargeDashState>(owner);

	if (chargeDashState == nullptr) { return 0; }

	if (chargeDashState->IsDashCharged() && mCurrentDashTimer >= mMaxDashTime)
	{
		return 0.9f;
	}

	return 0;
}

void Game::DashingState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->StopNavMesh();
}

bool Game::DashingState::IsDashCharged() const
{
	return mCurrentDashTimer >= mMaxDashTime;
}

std::pair<float, entt::entity> Game::DashingState::GetBestScoreAndTarget(const CE::World& world,
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

CE::MetaType Game::DashingState::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<DashingState>{}, "DashingState" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&DashingState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DashingState::mSpeedDash, "mSpeedDash").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &DashingState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &DashingState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &DashingState::OnAIStateEnterEvent);

	type.AddField(&DashingState::mDashingAnimation, "mAttackingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<DashingState>(type);
	return type;
}
