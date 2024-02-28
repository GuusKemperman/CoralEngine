#include "Precomp.h"
#include "NavMeshTargetComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

using namespace Engine;

NavMeshTargetComponent::NavMeshTargetComponent()
{
}

MetaType NavMeshTargetComponent::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshTargetComponent>{}, "NavMeshTargetComponent"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<NavMeshTargetComponent>(metaType);

	return metaType;
}
