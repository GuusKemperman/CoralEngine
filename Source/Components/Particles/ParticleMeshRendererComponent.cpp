#include "Precomp.h"
#include "Components/Particles/ParticleMeshRendererComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Assets/StaticMesh.h"
#include "Assets/Material.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleUtilities.h"

bool CE::ParticleMeshRendererComponent::AreAnyVisible(const CE::ParticleEmitterComponent& emitter) const
{
	return emitter.GetNumOfParticles() != 0 && mParticleMesh != nullptr && mParticleMaterial != nullptr; 
}

CE::MetaType CE::ParticleMeshRendererComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleMeshRendererComponent>{}, "ParticleMeshRendererComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleMeshRendererComponent::mParticleMesh, "mParticleMesh").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleMeshRendererComponent::mParticleMaterial, "mParticleMaterial").GetProperties().Add(Props::sIsScriptableTag);
	ReflectParticleComponentType<ParticleMeshRendererComponent>(type);
	return type;
}
