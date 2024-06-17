#include "Precomp.h"
#include "Components/ToneMappingComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::ToneMappingComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<ToneMappingComponent>{}, "ToneMappingComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&ToneMappingComponent::mExposure, "mExposure").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<ToneMappingComponent>(metaType);

	return metaType;
}
