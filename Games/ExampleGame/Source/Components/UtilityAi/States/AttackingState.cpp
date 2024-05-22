#include "Precomp.h"
#include "Components/UtililtyAi/States/AttackingState.h"

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

void Game::AttackingState::OnAiTick(CE::World& world, entt::entity owner, float)
{
	auto* animationRootComponent = world.GetRegistry().TryGet<CE::AnimationRootComponent>(owner);

	if (animationRootComponent != nullptr)
	{
		animationRootComponent->SwitchAnimation(world.GetRegistry(), mAttackingAnimation, 0.0f);
	}

	auto [score, targetEntity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = targetEntity;

	const auto characterData = world.GetRegistry().TryGet<CE::CharacterComponent>(owner);
	if (characterData == nullptr)
	{
		LOG(LogAI, Warning, "A character component is needed to run the Attacking State!");
		return;
	}

	const auto abilities = world.GetRegistry().TryGet<CE::AbilitiesOnCharacterComponent>(owner);
	if (abilities == nullptr)
	{
		LOG(LogAI, Warning, "A AbilitiesOnCharacter component is needed to run the Attacking State!");
		return;
	}

	if (abilities->mAbilitiesToInput.empty())
	{
		return;
	}

	CE::AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);

	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		return;
	}

	if (mTargetEntity != entt::null)
	{
		navMeshAgent->SetTargetEntity(mTargetEntity);
	}
}

float Game::AttackingState::OnAiEvaluate(const CE::World& world, entt::entity owner)
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = entity;
	return score;
}

void Game::AttackingState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr)
	{
		LOG(LogAI, Warning, "A NavMeshAgent component is needed to run the Attacking State!");
		return;
	}

	navMeshAgent->ClearTarget(world);
}

std::pair<float, entt::entity> Game::AttackingState::GetBestScoreAndTarget(const CE::World& world,
                                                                           entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	entt::entity entityId = world.GetRegistry().View<CE::NavMeshTargetTag>().front();

	if (entityId == entt::null)
	{
		LOG(LogAI, Warning, "An entity with a NavMeshTarget component is needed to run the Attacking State!");
		return { 0.0f, entt::null };
	}

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "A transform component is needed to run the Attacking State!");
		return { 0.0f, entt::null };
	}

	float highestScore = 0.0f;

	auto* targetComponent = world.GetRegistry().TryGet<CE::TransformComponent>(entityId);

	if (transformComponent == nullptr)
	{
		LOG(LogAI, Warning, "A transform component on the player entity is needed to run the Attacking State!");
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

CE::MetaType Game::AttackingState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<AttackingState>{}, "AttackingState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&AttackingState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &AttackingState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &AttackingState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &AttackingState::OnAIStateEnterEvent);

	type.AddField(&AttackingState::mAttackingAnimation, "mAttackingAnimation").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<AttackingState>(type);
	return type;
}
