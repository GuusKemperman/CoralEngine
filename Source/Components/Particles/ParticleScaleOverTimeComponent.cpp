#include "Precomp.h"
#include "Components/Particles/ParticleScaleOverTimeComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::ParticleScaleOverTimeComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleScaleOverTimeComponent>{}, "ParticleScaleOverTimeComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleScaleOverTimeComponent::mScaleMultiplierOverParticleLifeTime, "mScaleMultiplierOverParticleLifeTime");
	ReflectComponentType<ParticleScaleOverTimeComponent>(type);
	return type;
}
