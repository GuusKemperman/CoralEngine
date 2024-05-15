#include "Precomp.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectOptional.h"

std::optional<glm::vec2> CE::NavMeshAgentComponent::GetTargetPosition() const
{
	return mTargetPosition;
}

void CE::NavMeshAgentComponent::SetTargetPosition(glm::vec2 targetPosition)
{
	mIsChasing = true;
	mTargetPosition = targetPosition;
}

void CE::NavMeshAgentComponent::SetTargetPosition(const TransformComponent& transformComponent)
{
	mIsChasing = true;
	mTargetPosition = {transformComponent.GetWorldPosition().x, transformComponent.GetWorldPosition().z};
}

void CE::NavMeshAgentComponent::UpdateTargetPosition(glm::vec2 targetPosition)
{
	mTargetPosition = targetPosition;
}

void CE::NavMeshAgentComponent::UpdateTargetPosition(const TransformComponent& transformComponent)
{
	mTargetPosition = { transformComponent.GetWorldPosition().x, transformComponent.GetWorldPosition().z };
}

void CE::NavMeshAgentComponent::StopNavMesh()
{
	mIsChasing = false;
	mJustStopped = true;
}

bool CE::NavMeshAgentComponent::IsChasing() const
{
	return mIsChasing;
}

bool CE::NavMeshAgentComponent::WasJustStopped()
{
	if (mJustStopped == true)
	{
		mJustStopped = false;
		return true;
	}
	return false;
}

CE::MetaType CE::NavMeshAgentComponent::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshAgentComponent>{}, "NavMeshAgentComponent"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&NavMeshAgentComponent::mTargetPosition, "mTargetPosition").GetProperties();
	metaType.AddField(&NavMeshAgentComponent::mIsChasing, "mIsChasing").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	CE::ReflectComponentType<NavMeshAgentComponent>(metaType);

	return metaType;
}
