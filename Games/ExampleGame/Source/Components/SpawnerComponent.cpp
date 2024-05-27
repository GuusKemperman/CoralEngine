#include "Precomp.h"
#include "Components/SpawnerComponent.h"

#include "Meta/Fwd/MetaTypeFwd.h"
#include "Assets/Prefabs/Prefab.h"
#include "Utilities/Reflect/ReflectComponentType.h"


CE::MetaType Game::SpawnerComponent::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<SpawnerComponent>{}, "SpawnerComponent"};
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);

	type.AddField(&SpawnerComponent::mMinSpawnRange, "mMinSpawnRange").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mAmountToSpawnPerSecond, "mAmountToSpawnPerSecond").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mPrefabToSpawn, "mPrefabToSpawn").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<SpawnerComponent>(type);
	return type;
}
