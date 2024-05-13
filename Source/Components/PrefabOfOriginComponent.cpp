#include "Precomp.h"
#include "Components/PrefabOriginComponent.h"

#include "Assets/Prefabs/Prefab.h"
#include "Assets/Prefabs/PrefabEntityFactory.h"
#include "Core/AssetManager.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::PrefabOriginComponent::PrefabOriginComponent(Name::HashType hashedPrefabName, uint32 factoryId) :
	mHashedPrefabName(hashedPrefabName),
	mFactoryId(factoryId)
{
}

CE::PrefabOriginComponent::PrefabOriginComponent(const PrefabEntityFactory& factory)
{
	SetFactoryOfOrigin(factory);
}

void CE::PrefabOriginComponent::SetFactoryOfOrigin(const PrefabEntityFactory& factory)
{
	mHashedPrefabName = Name::HashString(factory.GetPrefab().GetName());
	mFactoryId = factory.GetId();
}

const CE::Prefab* CE::PrefabOriginComponent::TryGetPrefab() const
{
	return AssetManager::Get().TryGetAsset<Prefab>(mHashedPrefabName).Get();
}

const CE::PrefabEntityFactory* CE::PrefabOriginComponent::TryGetFactory() const
{
	const Prefab* prefab = TryGetPrefab();

	if (prefab == nullptr
		|| prefab->GetFactories().empty())
	{
		return nullptr;
	}

	// This is the root factory. The id
	// of the root factory is based on the
	// prefab name. This branch makes sure
	// references are not broken when the
	// prefab is renamed.
	if (mFactoryId == mHashedPrefabName)
	{
		return &prefab->GetFactories().front();
	}

	return prefab->TryFindFactory(mFactoryId);
}

CE::MetaType CE::PrefabOriginComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PrefabOriginComponent>{}, "PrefabOriginComponent" };

	metaType.GetProperties().Add(Props::sNoInspectTag);

	metaType.AddField(&PrefabOriginComponent::mHashedPrefabName, "mHashedPrefabName").GetProperties().Add(Props::sNoInspectTag);
	metaType.AddField(&PrefabOriginComponent::mFactoryId, "mFactoryId").GetProperties().Add(Props::sNoInspectTag);

	ReflectComponentType<PrefabOriginComponent>(metaType);
	return metaType;
}
