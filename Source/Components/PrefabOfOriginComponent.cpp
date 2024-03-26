#include "Precomp.h"
#include "Components/PrefabOriginComponent.h"

#include "Assets/Prefabs/Prefab.h"
#include "Assets/Prefabs/PrefabEntityFactory.h"
#include "Core/AssetManager.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::PrefabOriginComponent::PrefabOriginComponent(Name::HashType hashedPrefabName, uint32 factoryId) :
	mHashedPrefabName(hashedPrefabName),
	mFactoryId(factoryId)
{
}

Engine::PrefabOriginComponent::PrefabOriginComponent(const PrefabEntityFactory& factory)
{
	SetFactoryOfOrigin(factory);
}

void Engine::PrefabOriginComponent::SetFactoryOfOrigin(const PrefabEntityFactory& factory)
{
	mHashedPrefabName = Name::HashString(factory.GetPrefab().GetName());
	mFactoryId = factory.GetId();
}

const Engine::Prefab* Engine::PrefabOriginComponent::TryGetPrefab() const
{
	return AssetManager::Get().TryGetAsset<Prefab>(mHashedPrefabName).get();
}

const Engine::PrefabEntityFactory* Engine::PrefabOriginComponent::TryGetFactory() const
{
	const Prefab* prefab = TryGetPrefab();
	return prefab == nullptr ? nullptr : prefab->TryFindFactory(mFactoryId);
}

Engine::MetaType Engine::PrefabOriginComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PrefabOriginComponent>{}, "PrefabOriginComponent" };

	metaType.GetProperties().Add(Props::sNoInspectTag);

	metaType.AddField(&PrefabOriginComponent::mHashedPrefabName, "mHashedPrefabName").GetProperties().Add(Props::sNoInspectTag);
	metaType.AddField(&PrefabOriginComponent::mFactoryId, "mFactoryId").GetProperties().Add(Props::sNoInspectTag);

	ReflectComponentType<PrefabOriginComponent>(metaType);
	return metaType;
}
