#include "Precomp.h"
#include "Assets/Level.h"

#include <numeric>

#include "Core/AssetManager.h"
#include "World/World.h"
#include "World/Archiver.h"
#include "World/Registry.h"
#include "GSON/GSONBinary.h"
#include "Assets/Prefabs/Prefab.h"
#include "Components/PrefabOriginComponent.h"
#include "Components/TransformComponent.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Utilities/Reflect/ReflectAssetType.h"

namespace Engine
{
	static void EraseSerializedComponents(BinaryGSONObject& serializedWorld,
		const std::string& nameOfClassToErase,
		const std::vector<entt::entity>& eraseFromIds);

	static void EraseSerializedFactory(BinaryGSONObject& serializedComponents,
		const BinaryGSONObject& serializedFactory,
		const std::string& prefabName);

	static std::vector<entt::entity> GetIds(const BinaryGSONObject& serializedFactory);

	static std::vector<std::reference_wrapper<const BinaryGSONObject>> GetSerializedChildFactories(const BinaryGSONObject& serializedFactory);

	static std::vector<entt::entity> GetAllEntitiesCreatedUsingFactory(const BinaryGSONObject& serializedWorld,
		const std::string& prefabName,
		uint32 factoryId);
}

Engine::Level::Level(std::string_view name) :
	Asset(name, MakeTypeId<Level>()),
	mSerializedComponents(std::make_unique<BinaryGSONObject>())
{
}

Engine::Level::Level(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	BinaryGSONObject savedData{};
	savedData.LoadFromBinary(loadInfo.GetStream());

	BinaryGSONObject* const serializedWorld = savedData.TryGetGSONObject("Components");

	if (serializedWorld == nullptr)
	{
		LOG(LogAssets, Warning, "Invalid level {}: No components object serialized",
			GetName());
		return;
	}

	// Remove components that no longer exist
	std::vector<BinaryGSONObject>& serializedComponents = serializedWorld->GetChildren();
	serializedComponents.erase(std::remove_if(serializedComponents.begin(), serializedComponents.end(),
		[this](const BinaryGSONObject& serializedComponentClass)
		{
			const std::string& className = serializedComponentClass.GetName();
			const bool doesClassStillExist = MetaManager::Get().TryGetType(className) != nullptr;

			if (!doesClassStillExist)
			{
				LOG(LogAssets, Warning, "Level {} has components of a type {}, but this class no longer exists",
					GetName(),
					className);
			}

			return !doesClassStillExist;
		}), serializedComponents.end());

	BinaryGSONObject* const serializedPrefabs = savedData.TryGetGSONObject("Prefabs");
	if (serializedPrefabs == nullptr)
	{
		return;
	}

	std::vector<DiffedPrefab> diffedPrefabs{};

	for (BinaryGSONObject& serializedPrefab : serializedPrefabs->GetChildren())
	{
		diffedPrefabs.emplace_back(DiffPrefab(serializedPrefab));
	}

	// Erase individual components
	for (const DiffedPrefab& diffedPrefab : diffedPrefabs)
	{
		for (const DiffedPrefabFactory& diffedFactory : diffedPrefab.mDifferencesBetweenExistingFactories)
		{
			const std::vector<entt::entity> entitiesMadeWithThisFactory = GetIds(diffedFactory.mSerializedFactory);

			for (const std::string& componentClassName : diffedFactory.mNamesOfRemovedComponents)
			{
				EraseSerializedComponents(*serializedWorld, componentClassName, entitiesMadeWithThisFactory);
			}
		}
	}

	// Erase entire factories/prefabs
	for (const DiffedPrefab& diffedPrefab : diffedPrefabs)
	{
		for (const BinaryGSONObject& removedFactory : diffedPrefab.mSerializedFactoriesThatWereRemoved)
		{
			EraseSerializedFactory(*serializedWorld, removedFactory, diffedPrefab.mPrefabName);
		}
	}

	// Let's avoid deserializing the entire level unless needed
	class PossiblyWorld
	{
	public:
		PossiblyWorld(BinaryGSONObject& levelData) :
			mSerializedComponents(levelData) {}

		World& GetWorld()
		{
			if (mWorld == nullptr)
			{
				mWorld = std::make_unique<World>(false);
				Archiver::Deserialize(*mWorld, mSerializedComponents);
			}
			return *mWorld;
		}

		World* TryGetWorld()
		{
			return mWorld.get();
		}

	private:
		std::unique_ptr<World> mWorld{};
		BinaryGSONObject& mSerializedComponents;
	};
	PossiblyWorld possiblyWorld{ *serializedWorld };

	// Add entire factories/prefabs
	for (const DiffedPrefab& diffedPrefab : diffedPrefabs)
	{
		for (const PrefabEntityFactory& factoryToAdd : diffedPrefab.mAddedFactories)
		{
			const PrefabEntityFactory* const parent = factoryToAdd.GetParent();

			if (parent == nullptr)
			{
				LOG(LogAssets, Error, "While loading {}: An error occured with a prefab. The factory id of the root entity was changed somehow? This should not be possible. The level will not be loaded in correctly. Resolve the errors with the prefab and try again",
					GetName());
				continue;
			}

			Registry& reg = possiblyWorld.GetWorld().GetRegistry();
			const auto prefabOriginView = reg.View<TransformComponent, const PrefabOriginComponent>();

			for (auto [parentEntity, parentTransform, parentOrigin] : prefabOriginView.each())
			{
				const PrefabEntityFactory* factoryOfOrigin = parentOrigin.TryGetFactory();

				if (factoryOfOrigin == nullptr
					|| factoryOfOrigin != parent)
				{
					continue;
				}

				ASSERT(diffedPrefab.mPrefab.has_value());
				const entt::entity createdEntity = reg.CreateFromFactory(factoryToAdd, false);
				TransformComponent* myTransform = reg.TryGet<TransformComponent>(createdEntity);

				if (myTransform != nullptr)
				{
					myTransform->SetParent(&parentTransform);
				}
				else
				{
					LOG(LogAssets, Error, "A prefab had a parent child relationship, but atleast one of the entities does not have a transformComponent");
				}
			}
		}
	}

	// Add individual components
	for (const DiffedPrefab& diffedPrefab : diffedPrefabs)
	{
		for (const DiffedPrefabFactory& diffedFactory : diffedPrefab.mDifferencesBetweenExistingFactories)
		{
			for (const ComponentFactory& componentToAdd : diffedFactory.mAddedComponents)
			{
				World& world = possiblyWorld.GetWorld();
				Registry& reg = world.GetRegistry();

				const auto prefabOriginView = reg.View<const PrefabOriginComponent>();

				for (auto [entity, prefabOrigin] : prefabOriginView.each())
				{
					if (prefabOrigin.TryGetFactory() == &diffedFactory.mCurrentFactory.get())
					{
						componentToAdd.Construct(world.GetRegistry(), entity);
					}
				}
			}
		}
	}

	// The end goal is that level data will only contain the components object,
	// we used the prefabs object to compare the prefabs and no longer need it;

	if (const World* const world = possiblyWorld.TryGetWorld();
		world != nullptr)
	{
		mSerializedComponents = std::make_unique<BinaryGSONObject>(Archiver::Serialize(*world));
	}
	else
	{
		mSerializedComponents = std::make_unique<BinaryGSONObject>(std::move(*serializedWorld));
	}
}

Engine::Level::Level(Level&&) noexcept = default;

Engine::Level::~Level() = default;

void Engine::Level::OnSave(AssetSaveInfo& saveInfo) const
{
	if (mSerializedComponents == nullptr)
	{
		LOG(LogAssets, Error, "Cannot save level {}, mSerializedComponents is nullptr",
			GetName());
		return;
	}

	BinaryGSONObject tempObject{};
	std::vector<BinaryGSONObject>& children = tempObject.GetChildren();

	// Dont worry, we'll move it back later
	children.push_back(std::move(*mSerializedComponents));

	// We don't really care about the name anywhere else in the code,
	// so instead of making sure we always initialize it with the name
	// "Components", we just call it components right before saving.
	children.back().SetName("Components");

	children.emplace_back(GenerateCurrentStateOfPrefabs(children[0]));

	tempObject.SaveToBinary(saveInfo.GetStream());

	// See, it's all fine
	*mSerializedComponents = std::move(children[0]);
}

void Engine::Level::CreateFromWorld(const World& world)
{
	mSerializedComponents = std::make_unique<BinaryGSONObject>(Archiver::Serialize(world));
}

Engine::World Engine::Level::CreateWorld(const bool callBeginPlayImmediately) const
{
	World world{ false };

	if (mSerializedComponents == nullptr)
	{
		if (callBeginPlayImmediately)
		{
			world.BeginPlay();
		}
		return world;
	}

	Archiver::Deserialize(world, *mSerializedComponents);

	if (callBeginPlayImmediately)
	{
		world.BeginPlay();
	}

	return world;
}

std::vector<Engine::EntityType> GetIds(const Engine::BinaryGSONObject& serializedFactory)
{
	const Engine::BinaryGSONMember* serializedIds = serializedFactory.TryGetGSONMember("IDS");

	std::vector<Engine::EntityType> ids;
	if (serializedIds != nullptr)
	{
		*serializedIds >> ids;
	}
	return ids;
}

void Engine::EraseSerializedComponents(BinaryGSONObject& serializedWorld,
	const std::string& nameOfClassToErase,
	const std::vector<entt::entity>& eraseFromIds)
{
	for (BinaryGSONObject& serializedComponentClass : serializedWorld.GetChildren())
	{
		if (serializedComponentClass.GetName() != nameOfClassToErase)
		{
			continue;
		}

		std::vector<BinaryGSONObject>& serializedComponents = serializedComponentClass.GetChildren();

		serializedComponents.erase(std::remove_if(serializedComponents.begin(), serializedComponents.end(),
			[eraseFromIds](const BinaryGSONObject& serializedComponent)
			{
				const entt::entity ownerOfSerializedComponent = FromBinary<entt::entity>(serializedComponent.GetName());
				return std::find(eraseFromIds.begin(), eraseFromIds.end(), ownerOfSerializedComponent) != eraseFromIds.end();
			}), serializedComponents.end());
	}
}

void Engine::EraseSerializedFactory(BinaryGSONObject& serializedWorld,
	const BinaryGSONObject& serializedFactory,
	const std::string& prefabName)
{
	const std::vector<entt::entity> entitiesFromThisFactory = GetAllEntitiesCreatedUsingFactory(serializedWorld,
		prefabName,
		FromBinary<uint32>(serializedFactory.GetName()));

	BinaryGSONMember* serializedEntities = serializedWorld.TryGetGSONMember("entities");

	if (serializedEntities == nullptr)
	{
		LOG(LogAssets, Error, "Could not erase serialized factory, world does not contain an \"entities\" member");
		return;
	}

	std::vector<entt::entity> allEntities{};
	*serializedEntities >> allEntities;

	allEntities.erase(std::remove_if(allEntities.begin(), allEntities.end(),
		[&entitiesFromThisFactory](const entt::entity entity)
		{
			return std::find(entitiesFromThisFactory.begin(), entitiesFromThisFactory.end(), entity) != entitiesFromThisFactory.end();
		}), allEntities.end());

	*serializedEntities << allEntities;

	const BinaryGSONMember* const serializedComponentNames = serializedFactory.TryGetGSONMember("Components");

	if (serializedComponentNames != nullptr)
	{
		std::vector<std::string> componentNames{};
		*serializedComponentNames >> componentNames;

		for (const std::string& componentName : componentNames)
		{
			EraseSerializedComponents(serializedWorld, componentName, entitiesFromThisFactory);
		}
	}

	const MetaType& prefabOriginType = MetaManager::Get().GetType<PrefabOriginComponent>();
	EraseSerializedComponents(serializedWorld, prefabOriginType.GetName(), entitiesFromThisFactory);

	const auto children = GetSerializedChildFactories(serializedFactory);

	for (const BinaryGSONObject& serializedChildFactory : children)
	{
		EraseSerializedFactory(serializedWorld, serializedChildFactory, prefabName);
	}
}

namespace Engine
{
	BinaryGSONObject& GetSerializedPrefabFactory(BinaryGSONObject& prefabObject, const PrefabEntityFactory& prefabFactory);

	BinaryGSONObject& GetOrAddSerializedPrefabObject(BinaryGSONObject& prefabsObject, const Prefab& prefab);
}

Engine::Level::DiffedPrefabFactory Engine::Level::DiffPrefabFactory(const BinaryGSONObject& serializedFactory, 
	const PrefabEntityFactory& currentVersion)
{
	DiffedPrefabFactory returnValue{ currentVersion, serializedFactory };
	const std::vector<ComponentFactory>& currentComponents = currentVersion.GetComponentFactories();

	std::vector<std::string> componentNames{};

	if (const BinaryGSONMember* const serializedComponentNames = serializedFactory.TryGetGSONMember("Components");
		serializedComponentNames != nullptr)
	{
		*serializedComponentNames >> componentNames;
	}
	else
	{
		LOG(LogAssets, Error, "Missing 'Components' object in level {}", GetName());
	}

	std::vector<uint32> indicesOfComponentNames(componentNames.size());
	std::iota(indicesOfComponentNames.begin(), indicesOfComponentNames.end(), 0);

	for (const ComponentFactory& componentFactory : currentComponents)
	{
		const std::string& className = componentFactory.GetProductClass().GetName();

		auto serializedCounterpart = std::find_if(indicesOfComponentNames.begin(), indicesOfComponentNames.end(),
			[&className, &componentNames](const uint32 index)
			{
				return className == componentNames[index];
			});

		if (serializedCounterpart != indicesOfComponentNames.end())
		{
			// Prevents us searching through this entry again, and makes it easy to see which 
			// components are not currently present, they'll be all that are left once after 
			// we're done iterating over all our current factories
			*serializedCounterpart = indicesOfComponentNames.back();
			indicesOfComponentNames.pop_back();
		}
		else
		{
			// At the time of saving, this prefab did not have this component. Now that we are deserializing,
			// lets make sure that we add the component.
			returnValue.mAddedComponents.emplace_back(componentFactory);
		}
	}

	returnValue.mNamesOfRemovedComponents.reserve(indicesOfComponentNames.size());
	for (const uint32 indexOfRemovedComponnent : indicesOfComponentNames)
	{
		returnValue.mNamesOfRemovedComponents.emplace_back(componentNames[indexOfRemovedComponnent]);
	}

	return returnValue;
}

Engine::Level::DiffedPrefab Engine::Level::DiffPrefab(const BinaryGSONObject& serializedPrefab)
{
	const std::string& prefabName = serializedPrefab.GetName();
	DiffedPrefab returnValue{ serializedPrefab.GetName() };

	const std::shared_ptr<const Prefab> prefab = AssetManager::Get().TryGetAsset<Prefab>(prefabName);
	const std::vector<BinaryGSONObject>& serializedFactories = serializedPrefab.GetChildren();

	if (prefab == nullptr)
	{
		LOG(LogAssets, Message, "Level {} contains Prefab {}, which no longer exists.",
			GetName(), prefabName);

		for (const BinaryGSONObject& factoryToErase : serializedFactories)
		{
			returnValue.mSerializedFactoriesThatWereRemoved.emplace_back(factoryToErase);
		}

		return returnValue;
	}

	returnValue.mPrefab = prefab;

	const std::vector<PrefabEntityFactory>& currentFactories = prefab->GetFactories();

	std::vector<uint32> indicesOfSerializedFactories(serializedFactories.size());
	std::iota(indicesOfSerializedFactories.begin(), indicesOfSerializedFactories.end(), 0);

	for (const PrefabEntityFactory& factory : currentFactories)
	{
		const std::string serializedFactoryId = ToBinary(factory.GetId());

		auto serializedCounterpart = std::find_if(indicesOfSerializedFactories.begin(), indicesOfSerializedFactories.end(),
			[&serializedFactoryId, &serializedFactories](const uint32 serializedFactoryIndex)
			{
				return serializedFactories[serializedFactoryIndex].GetName() == serializedFactoryId;
			});

		if (serializedCounterpart != indicesOfSerializedFactories.end())
		{
			returnValue.mDifferencesBetweenExistingFactories.emplace_back(DiffPrefabFactory(serializedFactories[*serializedCounterpart], factory));

			// Prevents us searching through this entry again, and makes it easy to see which 
			// factories are not currently present, they'll be all that are left once after 
			// we're done iterating over all our current factories
			*serializedCounterpart = indicesOfSerializedFactories.back();
			indicesOfSerializedFactories.pop_back();
		}
		else
		{
			// At the time of serialization, this prefab did not have a child with this name.
			// We'll have to add a child to each instance of this prefab
			returnValue.mAddedFactories.push_back(factory);
		}
	}

	// Remove any children that the prefab had at the time of saving, but no longer has.
	for (const uint32& factoryToEraseIndex : indicesOfSerializedFactories)
	{
		returnValue.mSerializedFactoriesThatWereRemoved.emplace_back(serializedFactories[factoryToEraseIndex]);
	}

	// Make sure we create each parent before its child
	std::sort(returnValue.mAddedFactories.begin(), returnValue.mAddedFactories.end(),
		[](const PrefabEntityFactory& left, const PrefabEntityFactory& right)
		{
			const PrefabEntityFactory* leftParent = left.GetParent();
			const PrefabEntityFactory* rightParent = right.GetParent();

			if (leftParent == nullptr)
			{
				return true;
			}

			if (rightParent == nullptr)
			{
				return false;
			}

			if (rightParent == &left)
			{
				return true;
			}

			if (leftParent == &right)
			{
				return false;
			}

			// We don't really care anymore, they are for our purposes equal,
			// but sort requires us to be consistent
			return leftParent < rightParent;
		});

	return returnValue;
}

Engine::BinaryGSONObject Engine::Level::GenerateCurrentStateOfPrefabs(const BinaryGSONObject& serializedWorld)
{
	BinaryGSONObject prefabsObject{ "Prefabs" };

	std::unordered_map<const PrefabEntityFactory*, std::vector<entt::entity>> idsPerFactory{};

	const MetaType& prefabOriginType = MetaManager::Get().GetType<PrefabOriginComponent>();

	const BinaryGSONObject* const serializedPrefabOrigins = serializedWorld.TryGetGSONObject(prefabOriginType.GetName());

	if (serializedPrefabOrigins == nullptr)
	{
		return prefabsObject;
	}

	for (const BinaryGSONObject& child : serializedPrefabOrigins->GetChildren())
	{
		const entt::entity entity = FromBinary<entt::entity>(child.GetName());

		const BinaryGSONMember* const serializedHashedPrefabName = child.TryGetGSONMember(ToBinary(Name::HashString("mHashedPrefabName")));
		const BinaryGSONMember* const serializedFactoryId = child.TryGetGSONMember(ToBinary(Name::HashString("mFactoryId")));

		if (serializedHashedPrefabName == nullptr
			|| serializedFactoryId == nullptr)
		{
			LOG(LogAssets, Warning, "Prefab origin component not serialized as expected");
			continue;
		}

		Name::HashType hashedPrefabName{};

		*serializedHashedPrefabName >> hashedPrefabName;

		const Prefab* prefab = AssetManager::Get().TryGetAsset<Prefab>(hashedPrefabName).get();

		if (prefab == nullptr)
		{
			continue;
		}

		uint32 factoryId{};
		*serializedFactoryId >> factoryId;

		const PrefabEntityFactory* factory = prefab->TryFindFactory(factoryId);

		if (factory != nullptr)
		{
			idsPerFactory[factory].emplace_back(entity);
		}
	}

	for (const auto& [factory, prefabIdsPair] : idsPerFactory)
	{
		BinaryGSONObject& serializedPrefab = GetOrAddSerializedPrefabObject(prefabsObject, factory->GetPrefab());
		BinaryGSONObject& serializedFactory = GetSerializedPrefabFactory(serializedPrefab, *factory);
		serializedFactory.AddGSONMember("IDS") << prefabIdsPair;
	}
	
	return prefabsObject;
}

std::vector<entt::entity> Engine::GetAllEntitiesCreatedUsingFactory(const BinaryGSONObject& serializedWorld,
	const std::string& prefabName,
	const uint32 factoryId)
{
	const MetaType& prefabOriginType = MetaManager::Get().GetType<PrefabOriginComponent>();

	const BinaryGSONObject* const serializedPrefabOrigins = serializedWorld.TryGetGSONObject(prefabOriginType.GetName());

	std::vector<entt::entity> ids{};

	if (serializedPrefabOrigins == nullptr)
	{
		return ids;
	}

	ids.reserve(serializedPrefabOrigins->GetChildren().size());

	const std::string prefabHashNameAsString = ToBinary(Name::HashString(prefabName));
	const std::string factoryIdAsString = ToBinary(factoryId);

	for (const BinaryGSONObject& serializedPrefabOrigin : serializedPrefabOrigins->GetChildren())
	{
		const BinaryGSONMember* const serializedHashedPrefabName = serializedPrefabOrigin.TryGetGSONMember(ToBinary(Name::HashString("mHashedPrefabName")));
		const BinaryGSONMember* const serializedFactoryId = serializedPrefabOrigin.TryGetGSONMember(ToBinary(Name::HashString("mFactoryId")));

		if (serializedHashedPrefabName == nullptr
			|| serializedFactoryId == nullptr)
		{
			LOG(LogAssets, Warning, "Prefab origin component not serialized as expected");
			continue;
		}

		if (serializedHashedPrefabName->GetData() == prefabHashNameAsString
			&& serializedFactoryId->GetData() == factoryIdAsString)
		{
			ids.emplace_back(FromBinary<entt::entity>(serializedPrefabOrigin.GetName()));
		}
	}

	return ids;
}

std::vector<entt::entity> Engine::GetIds(const BinaryGSONObject& serializedFactory)
{
	const BinaryGSONMember* serializedIds = serializedFactory.TryGetGSONMember("IDS");

	std::vector<entt::entity> ids;
	if (serializedIds != nullptr)
	{
		*serializedIds >> ids;
	}
	return ids;
}

std::vector<std::reference_wrapper<const Engine::BinaryGSONObject>> Engine::GetSerializedChildFactories(const BinaryGSONObject& serializedFactory)
{
	std::vector<std::reference_wrapper<const BinaryGSONObject>> returnValue{};

	// The parent object contains all the factories, including this one
	for (const BinaryGSONObject& possibleChild : serializedFactory.GetParent()->GetChildren())
	{
		const BinaryGSONMember* serializedParentId = possibleChild.TryGetGSONMember("ParentId");

		// We are assuming the factory id got saved in binary format. 
		// If that's true, the size of both strings should be equal.
		ASSERT(serializedParentId == nullptr ||
			(serializedParentId->GetData().size() == serializedFactory.GetName().size()
				&& serializedFactory.GetName().size() == sizeof(uint32)));

		// We don't even need to deserialize the id, because the name
		// of the serialized factory object is its id serialized.
		if (serializedParentId != nullptr
			&& serializedParentId->GetData() == serializedFactory.GetName())
		{
			returnValue.emplace_back(possibleChild);
		}
	}

	return returnValue;
}

Engine::BinaryGSONObject& Engine::GetSerializedPrefabFactory(BinaryGSONObject& prefabObject, const PrefabEntityFactory& prefabFactory)
{
	std::function<BinaryGSONObject*(BinaryGSONObject&, std::string_view)> findRecursive = [&findRecursive](BinaryGSONObject& current, std::string_view toFind) -> BinaryGSONObject*
		{
			if (current.GetName() == toFind)
			{
				return &current;
			}

			BinaryGSONObject* insideMe = current.TryGetGSONObject(toFind);
				
			if (insideMe != nullptr)
			{
				return insideMe;
			}

			for (BinaryGSONObject& child : current.GetChildren())
			{
				BinaryGSONObject* insideChild = findRecursive(child, toFind);

				if (insideChild != nullptr)
				{
					return insideChild;
				}
			}

			return nullptr;
		};

	BinaryGSONObject* obj = findRecursive(prefabObject, ToBinary(prefabFactory.GetId()));
	ASSERT(obj != nullptr);
	return *obj;
}

Engine::BinaryGSONObject& Engine::GetOrAddSerializedPrefabObject(BinaryGSONObject& prefabsObject, const Prefab& prefab)
{
	const std::string& prefabName = prefab.GetName();
	BinaryGSONObject* serializedPrefab = prefabsObject.TryGetGSONObject(prefabName);

	if (serializedPrefab == nullptr) // Add the prefab if we haven't already
	{
		serializedPrefab = &prefabsObject.AddGSONObject(prefabName);

		for (const PrefabEntityFactory& factory : prefab.GetFactories())
		{
			BinaryGSONObject& factoryObject = serializedPrefab->AddGSONObject(ToBinary(factory.GetId()));

			std::vector<std::string> componentNames{};
			for (const ComponentFactory& componentFactory : factory.GetComponentFactories())
			{
				// By keeping track of each component this prefab has at the time of saving, we can 
				// detect if the prefab asset has been edited to contain an additional component. We 
				// can then add that component to each serialized entity while deserializing.
				componentNames.push_back(componentFactory.GetProductClass().GetName());
			}
			factoryObject.AddGSONMember("Components") << componentNames;
			
			if (factory.GetParent() != nullptr)
			{
				factoryObject.AddGSONMember("ParentId") << factory.GetParent()->GetId();
			}
		}
	}

	return *serializedPrefab;
}

Engine::MetaType Engine::Level::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Level>{}, "Level", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{} };
	ReflectAssetType<Level>(type);
	return type;
}