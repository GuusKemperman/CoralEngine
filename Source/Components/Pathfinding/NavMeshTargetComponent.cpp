#include "Precomp.h"
#include "Components/Pathfinding/NavMeshTargetComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

using namespace Engine;

NavMeshTargetTag::NavMeshTargetTag()
{
}

MetaType NavMeshTargetTag::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshTargetTag>{}, "NavMeshTargetComponent"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<NavMeshTargetTag>(metaType);

	return metaType;
}
