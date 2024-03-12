#include "Precomp.h"
#include "Components/DirectionalLightComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::DirectionalLightComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DirectionalLightComponent>{}, "DirectionalLightComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&DirectionalLightComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&DirectionalLightComponent::mIntensity, "mIntensity").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<DirectionalLightComponent>(metaType);

	return metaType;
}
