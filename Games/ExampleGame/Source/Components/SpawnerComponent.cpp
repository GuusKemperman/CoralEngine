#include "Precomp.h"
#include "Components/SpawnerComponent.h"

#include "Meta/Fwd/MetaTypeFwd.h"
#include "Assets/Prefabs/Prefab.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"
#include "Utilities/Reflect/ReflectComponentType.h"


Engine::MetaType Game::SpawnerComponent::Reflect()
{
	auto type = Engine::MetaType{Engine::MetaType::T<SpawnerComponent>{}, "SpawnerComponent"};
	Engine::MetaProps& props = type.GetProperties();
	props.Add(Engine::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mSpawningTimer, "SpawningTimer").GetProperties().Add(
		Engine::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mEnemyPrefab, "EnemyPrefab").GetProperties().Add(Engine::Props::sIsScriptableTag);
	Engine::ReflectComponentType<SpawnerComponent>(type);
	return type;
}
