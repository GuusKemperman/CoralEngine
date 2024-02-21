#include "Precomp.h"
#include "Components/Particles/ParticleColorOverTimeComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::ParticleColorOverTimeComponent::Reflect()
{
	MetaType type = MetaType{MetaType::T<ParticleColorOverTimeComponent>{}, "ParticleColorOverTimeComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleColorOverTimeComponent::mGradient, "mGradient");
	ReflectComponentType<ParticleColorOverTimeComponent>(type);
	return type;
}
