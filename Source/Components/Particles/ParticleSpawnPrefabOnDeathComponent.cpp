#include "Precomp.h"
#include "Components/Particles/ParticleSpawnPrefabOnDeathComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Assets/Prefabs/Prefab.h"
#include "Components/Particles/ParticleUtilities.h"

CE::MetaType CE::ParticleSpawnPrefabOnDeathComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleSpawnPrefabOnDeathComponent>{}, "ParticleSpawnPrefabOnDeathComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleSpawnPrefabOnDeathComponent::mPrefabToSpawn, "mPrefabToSpawn").GetProperties().Add(Props::sIsScriptableTag);
	ReflectParticleComponentType<ParticleSpawnPrefabOnDeathComponent>(type);
	return type;
}
