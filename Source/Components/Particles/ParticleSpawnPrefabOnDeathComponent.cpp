#include "Precomp.h"
#include "Components/Particles/ParticleSpawnPrefabOnDeathComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Prefabs/Prefab.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"

Engine::MetaType Engine::ParticleSpawnPrefabOnDeathComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ParticleSpawnPrefabOnDeathComponent>{}, "ParticleSpawnPrefabOnDeathComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&ParticleSpawnPrefabOnDeathComponent::mPrefabToSpawn, "mPrefabToSpawn").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<ParticleSpawnPrefabOnDeathComponent>(type);
	return type;
}
