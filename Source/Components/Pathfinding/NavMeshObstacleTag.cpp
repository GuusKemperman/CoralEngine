#include "Precomp.h"
#include "Components/Pathfinding/NavMeshObstacleTag.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

using namespace Engine;

NavMeshObstacleTag::NavMeshObstacleTag()
{
}

MetaType NavMeshObstacleTag::Reflect()
{
	auto metaType = MetaType{MetaType::T<NavMeshObstacleTag>{}, "NavMeshObstacleComponent"};
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<NavMeshObstacleTag>(metaType);

	return metaType;
}
