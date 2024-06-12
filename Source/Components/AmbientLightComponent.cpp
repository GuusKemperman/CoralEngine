#include "Precomp.h"
#include "Components/AmbientLightComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::AmbientLightComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AmbientLightComponent>{}, "AmbientLightComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&AmbientLightComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AmbientLightComponent::mIntensity, "mIntensity").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<AmbientLightComponent>(metaType);

	return metaType;
}
