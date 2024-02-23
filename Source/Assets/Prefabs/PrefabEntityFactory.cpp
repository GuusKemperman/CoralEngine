#include "Precomp.h"
#include "Assets/Prefabs/PrefabEntityFactory.h"

#include "Assets/Prefabs/Prefab.h"
#include "GSON/GSONBinary.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaManager.h"

Engine::PrefabEntityFactory::PrefabEntityFactory(Prefab& prefab, const BinaryGSONObject& allSerializedComponents,
	entt::entity entityIdInSerializedComponents, uint32 factoryId, const PrefabEntityFactory* parent,
	std::vector<std::reference_wrapper<const PrefabEntityFactory>>&& children) :
	mPrefab(prefab),
	mParent(parent),
	mChildren(std::move(children)),
	mId(factoryId)
{
	const std::string entityAsString = ToBinary(entityIdInSerializedComponents);

	for (const BinaryGSONObject& serializedComponentClass : allSerializedComponents.GetChildren())
	{
		const BinaryGSONObject* serializedComponent = serializedComponentClass.TryGetGSONObject(entityAsString);

		if (serializedComponent == nullptr)
		{
			continue;
		}

		const MetaType* const objectClass = MetaManager::Get().TryGetType(serializedComponentClass.GetName());

		if (objectClass != nullptr)
		{
			mComponentFactories.emplace_back(*objectClass, *serializedComponent);
		}
		else
		{
			LOG_FMT(Assets, Warning,
				"While compiling prefab {}: Could not load component {}, as there is no class with this name anymore",
				prefab.GetName(), serializedComponentClass.GetName());
		}
	}
}

const Engine::ComponentFactory* Engine::PrefabEntityFactory::TryGetComponentFactory(const MetaType& objectClass) const
{
	const auto it = std::find_if(mComponentFactories.begin(), mComponentFactories.end(),
	                             [&objectClass](const ComponentFactory& factory)
	                             {
		                             return factory.GetProductClass() == objectClass;
	                             });
	return it == mComponentFactories.end() ? nullptr : &*it;
}
