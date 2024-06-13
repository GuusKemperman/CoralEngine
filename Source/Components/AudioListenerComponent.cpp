#include "Precomp.h"
#include "Components/AudioListenerComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::AudioListenerComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<AudioListenerComponent>{}, "AudioListenerComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&AudioListenerComponent::mVolume, "mVolume").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<AudioListenerComponent>(type);

	return type;
}
