#include "Precomp.h"
#include "Components/UtililtyAi/States/AttackingState.h"

#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshTargetComponent.h"
#include "Systems/AbilitySystem.h"
#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Game::AttackingState::OnAiTick(CE::World& world, entt::entity owner, float)
{
	auto [score, targetEntity] = GetBestScoreAndTarget(world, owner);
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

	CE::AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);

	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	if (mTargetEntity != entt::null)
	{
		const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(mTargetEntity);

		if (transformComponent == nullptr) { return; }

		navMeshAgent->UpdateTargetPosition(*transformComponent);
	}
}

float Game::AttackingState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);
	return score;
}

void Game::AttackingState::OnAIStateEnterEvent(CE::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->StopNavMesh();
}

std::pair<float, entt::entity> Game::AttackingState::GetBestScoreAndTarget(const CE::World& world,
                                                                           entt::entity owner) const
{
	const auto targetsView = world.GetRegistry().View<CE::NavMeshTargetTag, CE::TransformComponent>();
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		return {0.0f, entt::null};
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
			score = 10.0f;
		}

		if (highestScore < score)
		{
			highestScore = score;
			entityId = targetId;
		}
	}

	return {highestScore, entityId};
}

CE::MetaType Game::AttackingState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<AttackingState>{}, "AttackingState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&AttackingState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &AttackingState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &AttackingState::OnAiEvaluate);
	BindEvent(type, CE::sAIStateEnterEvent, &AttackingState::OnAIStateEnterEvent);

	CE::ReflectComponentType<AttackingState>(type);
	return type;
}
