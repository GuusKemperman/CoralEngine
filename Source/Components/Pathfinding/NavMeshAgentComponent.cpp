#include "Precomp.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

Engine::NavMeshAgentComponent::NavMeshAgentComponent(const float walkingSpeed) : Speed(walkingSpeed)
{
}

float Engine::NavMeshAgentComponent::GetSpeed() const
{
	return Speed;
}

Engine::MetaType Engine::NavMeshAgentComponent::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshAgentComponent>{}, "NavMeshAgentComponent"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);
	MetaProps& props = metaType.GetProperties();
	props.Add(Props::sIsScriptableTag);
	metaType.AddField(&NavMeshAgentComponent::Speed, "Speed").GetProperties().Add(Props::sIsScriptableTag);
	Engine::ReflectComponentType<NavMeshAgentComponent>(metaType);

	return metaType;
}
