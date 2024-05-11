#include "Precomp.h"
#include "Components/Particles/ParticleMeshRendererComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/StaticMesh.h"
#include "Assets/Material.h"

CE::MetaType CE::ParticleMeshRendererComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleMeshRendererComponent>{}, "ParticleMeshRendererComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleMeshRendererComponent::mParticleMesh, "mParticleMesh").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&ParticleMeshRendererComponent::mParticleMaterial, "mParticleMaterial").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<ParticleMeshRendererComponent>(type);
	return type;
}
