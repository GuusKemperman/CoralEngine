#include "Precomp.h"
#include "Components/StaticMeshComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/StaticMesh.h"
#include "Assets/Material.h"

CE::MetaType CE::StaticMeshComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<StaticMeshComponent>{}, "StaticMeshComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&StaticMeshComponent::mStaticMesh, "mStaticMesh").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&StaticMeshComponent::mMaterial, "mMaterial").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&StaticMeshComponent::mTilesWithMeshScale, "mTilesWithMeshScale").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&StaticMeshComponent::mTiling, "mTiling").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&StaticMeshComponent::mHighlightedMesh, "mHighlighted").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<StaticMeshComponent>(type);
	return type;
}
