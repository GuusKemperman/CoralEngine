#include "Precomp.h"
#include "Components/Particles/ParticleColorComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Components/Particles/ParticleProperty.h"

CE::MetaType CE::ParticleColorComponent::Reflect()
{
	MetaType type = MetaType{MetaType::T<ParticleColorComponent>{}, "ParticleColorComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleColorComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<ParticleColorComponent>(type);
	return type;
}
