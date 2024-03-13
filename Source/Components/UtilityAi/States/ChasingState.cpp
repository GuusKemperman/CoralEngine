#include "Precomp.h"
#include "Components/UtililtyAi/States/ChasingState.h"

#include "Components/TransformComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshTargetComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/DebugRenderer.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::ChasingState::OnAiTick(World& world, entt::entity owner, float)
{
	auto* navMeshAgent = world.GetRegistry().TryGet<NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	if (mChosenTargetEntity == entt::null)
	{
		mChosenTargetEntity = mTargetEntity;
	}

	if (mTargetEntity != entt::null)
	{
		const auto* transformComponent = world.GetRegistry().TryGet<TransformComponent>(mTargetEntity);

		if (transformComponent == nullptr) { return; }

		navMeshAgent->SetTarget(*transformComponent);
	}
}

float Engine::ChasingState::OnAiEvaluate(const World& world, entt::entity owner) const
{
	auto [score, entity] = GetHighestScore(world, owner);
	return score;
}

void Engine::ChasingState::OnAIStateEnterEvent(const World& world, entt::entity owner)
{
	auto [score, targetEntity] = GetHighestScore(world, owner);

	mTargetEntity = targetEntity;
}

std::pair<float, entt::entity> Engine::ChasingState::GetHighestScore(const World& world, entt::entity owner) const
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
			score = 5.f;
		}

		score = std::max(0.0f, std::min(mRadius, score));

		if (highestScore < score)
		{
			highestScore = score;
			entityId = targetId;
		}
	}

	return {highestScore, entityId};
}

void Engine::ChasingState::DebugRender(World& world, entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		return;
	}

	world.GetDebugRenderer().AddCircle(DebugCategory::Gameplay, transformComponent->GetWorldPosition(),
	                                   mRadius, {0.f, 0.f, 1.f, 1.f});
}

Engine::MetaType Engine::ChasingState::Reflect()
{
	auto type = MetaType{MetaType::T<ChasingState>{}, "ChasingState"};
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddField(&ChasingState::mRadius, "mRadius").GetProperties().Add(Props::sIsScriptableTag);

	BindEvent(type, sAITickEvent, &ChasingState::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &ChasingState::OnAiEvaluate);
	BindEvent(type, sAIStateEnterEvent, &ChasingState::OnAIStateEnterEvent);

	ReflectComponentType<ChasingState>(type);
	return type;
}
