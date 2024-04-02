#include "Precomp.h"
#include "Components/Pathfinding/NavMeshTargetComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"


CE::MetaType CE::NavMeshTargetTag::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshTargetTag>{}, "NavMeshTargetTag"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<NavMeshTargetTag>(metaType);

	return metaType;
}
