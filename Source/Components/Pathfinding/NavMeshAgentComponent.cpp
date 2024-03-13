#include "Precomp.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

float Engine::NavMeshAgentComponent::GetSpeed() const
{
	return mSpeed;
}

std::optional<glm::vec2> Engine::NavMeshAgentComponent::GetTargetPosition() const
{
	return mTargetPosition;
}

void Engine::NavMeshAgentComponent::SetTarget(glm::vec2 targetPosition)
{
	mTargetPosition = targetPosition;
}

void Engine::NavMeshAgentComponent::SetTarget(const TransformComponent& transformComponent)
{
	mTargetPosition = {transformComponent.GetWorldPosition().x, transformComponent.GetWorldPosition().z};
}

void Engine::NavMeshAgentComponent::StopNavMesh()
{
	mTargetPosition.reset();
}

Engine::MetaType Engine::NavMeshAgentComponent::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshAgentComponent>{}, "NavMeshAgentComponent"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&NavMeshAgentComponent::mSpeed, "Speed").GetProperties().Add(Props::sIsScriptableTag);
	Engine::ReflectComponentType<NavMeshAgentComponent>(metaType);

	return metaType;
}
