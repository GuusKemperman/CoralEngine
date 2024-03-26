#include "Precomp.h"
#include "Components/SpawnerComponent.h"

#include "Meta/Fwd/MetaTypeFwd.h"
#include "Assets/Prefabs/Prefab.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"
#include "Utilities/Reflect/ReflectComponentType.h"


Engine::MetaType Engine::SpawnerComponent::Reflect()
{
	auto type = MetaType{MetaType::T<SpawnerComponent>{}, "SpawnerComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mSpawningTimer, "SpawningTimer").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mEnemyPrefab, "EnemyPrefab").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<SpawnerComponent>(type);
	return type;
}
