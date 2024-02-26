#include "Precomp.h"
#include "Components/Physics2D/PolygonColliderComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

Engine::MetaType Engine::PolygonColliderComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PolygonColliderComponent>{}, "PolygonColliderComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&PolygonColliderComponent::mPoints, "mPoints").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<PolygonColliderComponent>(metaType);

	return metaType;
}
