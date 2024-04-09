#include "Precomp.h"
#include "Components/FogComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::FogComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<FogComponent>{}, "FogComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&FogComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&FogComponent::mNearPlane, "mNearPlane").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&FogComponent::mFarPlane, "mFarPlane").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<FogComponent>(metaType);

	return metaType;
}
