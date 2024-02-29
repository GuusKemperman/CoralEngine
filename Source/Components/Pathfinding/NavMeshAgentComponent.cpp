#include "Precomp.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

float Engine::NavMeshAgentComponent::GetSpeed() const
{
	return mSpeed;
}

Engine::MetaType Engine::NavMeshAgentComponent::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshAgentComponent>{}, "NavMeshAgentComponent"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&NavMeshAgentComponent::mSpeed, "Speed").GetProperties().Add(Props::sIsScriptableTag);
	Engine::ReflectComponentType<NavMeshAgentComponent>(metaType);

	return metaType;
}
