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

void Game::AttackingState::OnAiTick(Engine::World& world, entt::entity owner, float)
{
	auto [score, targetEntity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = targetEntity;

	const auto characterData = world.GetRegistry().TryGet<Engine::CharacterComponent>(owner);
	if (characterData == nullptr)
	{
		return;
	}

	const auto abilities = world.GetRegistry().TryGet<Engine::AbilitiesOnCharacterComponent>(owner);
	if (abilities == nullptr)
	{
		return;
	}

	if (abilities->mAbilitiesToInput.empty())
	{
		return;
	}

	Engine::AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);

	auto* navMeshAgent = world.GetRegistry().TryGet<Engine::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	if (mTargetEntity != entt::null)
	{
		const auto* transformComponent = world.GetRegistry().TryGet<Engine::TransformComponent>(mTargetEntity);

		if (transformComponent == nullptr) { return; }

		navMeshAgent->UpdateTargetPosition(*transformComponent);
	}
}

float Game::AttackingState::OnAiEvaluate(const Engine::World& world, entt::entity owner) const
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);
	return score;
}

void Game::AttackingState::OnAIStateEnterEvent(Engine::World& world, entt::entity owner)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<Engine::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	navMeshAgent->StopNavMesh();
}

std::pair<float, entt::entity> Game::AttackingState::GetBestScoreAndTarget(const Engine::World& world,
                                                                           entt::entity owner) const
{
	const auto targetsView = world.GetRegistry().View<Engine::NavMeshTargetTag, Engine::TransformComponent>();
	const auto* transformComponent = world.GetRegistry().TryGet<Engine::TransformComponent>(owner);

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

Engine::MetaType Game::AttackingState::Reflect()
{
	auto type = Engine::MetaType{Engine::MetaType::T<AttackingState>{}, "AttackingState"};
	type.GetProperties().Add(Engine::Props::sIsScriptableTag);

	type.AddField(&AttackingState::mRadius, "mRadius").GetProperties().Add(Engine::Props::sIsScriptableTag);

	BindEvent(type, Engine::sAITickEvent, &AttackingState::OnAiTick);
	BindEvent(type, Engine::sAIEvaluateEvent, &AttackingState::OnAiEvaluate);
	BindEvent(type, Engine::sAIStateEnterEvent, &AttackingState::OnAIStateEnterEvent);

	Engine::ReflectComponentType<AttackingState>(type);
	return type;
}
