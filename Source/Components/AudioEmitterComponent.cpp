#include "Precomp.h"
#include "Components/AudioEmitterComponent.h"

#include "Assets/Sound.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::AudioEmitterComponent::Play() const
{
	if (mSound != nullptr)
	{
		mSound->Play();
	}
}

CE::MetaType CE::AudioEmitterComponent::Reflect()
{
	auto type = MetaType{ MetaType::T<AudioEmitterComponent>{}, "AudioEmitterComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&AudioEmitterComponent::mSound, "mSound").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&AudioEmitterComponent::Play, "Play").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);

	ReflectComponentType<AudioEmitterComponent>(type);

	return type;
}
