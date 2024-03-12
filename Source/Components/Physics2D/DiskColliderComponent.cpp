#include "Precomp.h"
#include "Components/Physics2D/DiskColliderComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::DiskColliderComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DiskColliderComponent>{}, "DiskColliderComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&DiskColliderComponent::mRadius, "mRadius").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<DiskColliderComponent>(metaType);

	return metaType;
}
