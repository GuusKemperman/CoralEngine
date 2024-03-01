#include "Precomp.h"
#include "Components/PointLightComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::PointLightComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PointLightComponent>{}, "PointLightComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&PointLightComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PointLightComponent::mIntensity, "mIntensity").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PointLightComponent::mRange, "mRange").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<PointLightComponent>(metaType);

	return metaType;
}
