#include "Precomp.h"
#include "Components/Particles/ParticleLightComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Components/Particles/ParticleUtilities.h"

CE::MetaType CE::ParticleLightComponent::Reflect()
{
	MetaType type = MetaType{MetaType::T<ParticleLightComponent>{}, "ParticleLightComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleLightComponent::mIntensity, "mIntensity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleLightComponent::mRadius, "mRadius").GetProperties().Add(Props::sIsScriptableTag);
	ReflectParticleComponentType<ParticleLightComponent>(type);
	return type;
}
