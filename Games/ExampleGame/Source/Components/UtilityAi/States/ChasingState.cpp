#include "Precomp.h"
#include "Components/UtililtyAi/States/ChasingState.h"

#include "Components/TransformComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshTargetComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Game::ChasingState::OnAiTick(CE::World& world, entt::entity owner, float)
{
	auto [score, targetEntity] = GetBestScoreAndTarget(world, owner);
	mTargetEntity = targetEntity;

	auto* navMeshAgent = world.GetRegistry().TryGet<CE::NavMeshAgentComponent>(owner);

	if (navMeshAgent == nullptr) { return; }

	if (mTargetEntity != entt::null)
	{
		const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(mTargetEntity);

		if (transformComponent == nullptr) { return; }

		navMeshAgent->SetTargetPosition(*transformComponent);
	}
}

float Game::ChasingState::OnAiEvaluate(const CE::World& world, entt::entity owner) const
{
	auto [score, entity] = GetBestScoreAndTarget(world, owner);
	return score;
}

std::pair<float, entt::entity> Game::ChasingState::GetBestScoreAndTarget(const CE::World& world, entt::entity owner) const
{
	const auto targetsView = world.GetRegistry().View<CE::NavMeshTargetTag, CE::TransformComponent>();
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		return {0.0f, entt::null};
	}

	float highestScore{};
	entt::entity entityId{};

	for (auto [targetId, targetTransform] : targetsView.each())
	{
		const float distance = glm::distance(transformComponent->GetWorldPosition(),
		                                     targetTransform.GetWorldPosition());

		float score{};

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

void Game::ChasingState::DebugRender(CE::World& world, entt::entity owner) const
{
	const auto* transformComponent = world.GetRegistry().TryGet<CE::TransformComponent>(owner);

	if (transformComponent == nullptr)
	{
		return;
	}

	CE::DrawDebugCircle(
		world, CE::DebugCategory::Gameplay, 
		transformComponent->GetWorldPosition(),
	    mRadius, {0.f, 0.f, 1.f, 1.f});
}

CE::MetaType Game::ChasingState::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<ChasingState>{}, "ChasingState"};
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	type.AddField(&ChasingState::mRadius, "mRadius").GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAITickEvent, &ChasingState::OnAiTick);
	BindEvent(type, CE::sAIEvaluateEvent, &ChasingState::OnAiEvaluate);

	CE::ReflectComponentType<ChasingState>(type);
	return type;
}
