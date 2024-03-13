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

void Engine::AttackingState::OnAiTick(World& world, entt::entity owner, float)
{
	const auto characterData = world.GetRegistry().TryGet<CharacterComponent>(owner);
	if (characterData == nullptr)
	{
		return;
	}

	const auto abilities = world.GetRegistry().TryGet<AbilitiesOnCharacterComponent>(owner);
	if (abilities == nullptr)
	{
		return;
	}

	if (abilities->mAbilitiesToInput.empty())
	{
		return;
	}

	AbilitySystem::ActivateAbility(world, owner, *characterData, abilities->mAbilitiesToInput[0]);

	auto* navMeshAgent = world.GetRegistry().TryGet<NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	if (navMeshAgent->GetTargetPosition().has_value())
	{
		navMeshAgent->StopNavMesh();
	}
}

float Engine::AttackingState::OnAiEvaluate(const World& world, entt::entity owner) const
{
	auto [score, entity] = GetHighestScore(world, owner);
	return score;
}

void Engine::AttackingState::OnAIStateEnterEvent(const World& world, entt::entity owner)
{
	auto [score, targetEntity] = GetHighestScore(world, owner);
	mTargetEntity = targetEntity;
}


std::pair<float, entt::entity> Engine::AttackingState::GetHighestScore(const World& world, entt::entity owner) const
{
	const auto targetsView = world.GetRegistry().View<NavMeshTargetTag, TransformComponent>();
	const auto* transformComponent = world.GetRegistry().TryGet<TransformComponent>(owner);

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

Engine::MetaType Engine::AttackingState::Reflect()
{
	auto type = MetaType{MetaType::T<AttackingState>{}, "AttackingState"};
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&AttackingState::mRadius, "mRadius").GetProperties().Add(Props::sIsScriptableTag);

	BindEvent(type, sAITickEvent, &AttackingState::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &AttackingState::OnAiEvaluate);
	BindEvent(type, sAIStateEnterEvent, &AttackingState::OnAIStateEnterEvent);

	ReflectComponentType<AttackingState>(type);
	return type;
}
