#include "Precomp.h"
#include "Components/Particles/ParticleColorComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Components/Particles/ParticleUtilities.h"

CE::MetaType CE::ParticleColorComponent::Reflect()
{
	MetaType type = MetaType{MetaType::T<ParticleColorComponent>{}, "ParticleColorComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleColorComponent::mMultiplicativeColor, "mMultiplicativeColor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleColorComponent::mAdditiveColor, "mAdditiveColor").GetProperties().Add(Props::sIsScriptableTag);
	ReflectParticleComponentType<ParticleColorComponent>(type);
	return type;
}
