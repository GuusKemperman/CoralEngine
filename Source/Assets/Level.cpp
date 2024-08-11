#include "Precomp.h"
#include "Assets/Level.h"

#include <map>
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
#include "Components/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/NameComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Utilities/Reflect/ReflectAssetType.h"

namespace CE
{
	static void EraseSerializedComponents(World& world,
		const std::optional<std::string>& nameOfClassToErase, // If nullopt, will erase it from all component types
		const std::vector<entt::entity>& eraseFromIds);

	static void EraseSerializedFactory(World& world,
		const BinaryGSONObject& serializedFactory,
		const std::string& prefabName);

	static std::vector<entt::entity> GetIds(const BinaryGSONObject& serializedFactory);

	static std::vector<std::reference_wrapper<const BinaryGSONObject>> GetSerializedChildFactories(const BinaryGSONObject& serializedFactory);
}

CE::Level::Level(std::string_view name) :
	Asset(name, MakeTypeId<Level>()),
	mWorld(CreateDefaultWorld())
{
}

CE::Level::Level(AssetLoadInfo& loadInfo) :
	Asset(loadInfo),
	mWorld(false)
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

	Archiver::Deserialize(*mWorld, *serializedWorld);

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
			if (diffedFactory.mNamesOfRemovedComponents.empty())
			{
				continue;
			}

			const std::vector<entt::entity> entitiesMadeWithThisFactory = GetIds(diffedFactory.mSerializedFactory);

			for (const std::string& componentClassName : diffedFactory.mNamesOfRemovedComponents)
			{
				EraseSerializedComponents(*mWorld, componentClassName, entitiesMadeWithThisFactory);
			}
		}
	}

	// Erase entire factories/prefabs
	for (const DiffedPrefab& diffedPrefab : diffedPrefabs)
	{
		for (const BinaryGSONObject& removedFactory : diffedPrefab.mSerializedFactoriesThatWereRemoved)
		{
			EraseSerializedFactory(*mWorld, removedFactory, diffedPrefab.mPrefabName);
		}
	}

	mWorld->GetRegistry().RemovedDestroyed();

	// Add entire factories/prefabs
	for (const DiffedPrefab& diffedPrefab : diffedPrefabs)
	{
		for (const PrefabEntityFactory& factoryToAdd : diffedPrefab.mAddedFactories)
		{
			const PrefabEntityFactory* const parent = factoryToAdd.GetParent();

			if (parent == nullptr)
			{
				LOG(LogAssets, Error, "While loading {}: An error occured with a prefab. The factory id of the root entity was changed, the prefab has likely been renamed & resaved. Any instances in the level will no longer be updated if changes have been made to the prefab. This can be fixed by reverting the prefab to the name it had when you placed it into the level.",
					GetName());
				continue;
			}

			Registry& reg = mWorld->GetRegistry();
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
				Registry& reg = mWorld->GetRegistry();

				const auto prefabOriginView = reg.View<const PrefabOriginComponent>();

				for (auto [entity, prefabOrigin] : prefabOriginView.each())
				{
					if (prefabOrigin.TryGetFactory() == &diffedFactory.mCurrentFactory.get()
						&& !reg.HasComponent(componentToAdd.GetProductClass().GetTypeId(), entity))
					{
						componentToAdd.Construct(reg, entity);
					}
				}
			}
		}
	}
}

CE::Level::Level(Level&&) noexcept = default;

CE::Level::~Level() = default;

void CE::Level::OnSave(AssetSaveInfo& saveInfo) const
{
	if (!mSerializedWorld.has_value())
	{
		if (!mWorld.has_value())
		{
			LOG(LogAssets, Error, "Cannot save level {}, mSerializedComponents and mWorld are null",
				GetName());
			return;
		}

		mSerializedWorld.emplace(Archiver::Serialize(*mWorld));
	}

	BinaryGSONObject serializedLevel{};

	std::vector<BinaryGSONObject>& children = serializedLevel.GetChildren();
	children.reserve(2);

	children.emplace_back(std::move(*mSerializedWorld));

	// We don't really care about the name anywhere else in the code,
	// so instead of making sure we always initialize it with the name
	// "Components", we just call it components right before saving.
	children.back().SetName("Components");

	children.emplace_back(GenerateCurrentStateOfPrefabs(children[0]));

	serializedLevel.SaveToBinary(saveInfo.GetStream());

	mSerializedWorld = std::move(children[0]);
}

void CE::Level::CreateFromWorld(const World& world)
{
	mWorld.reset();
	mSerializedWorld.emplace(Archiver::Serialize(world));
}

CE::World CE::Level::CreateWorld(const bool callBeginPlayImmediately) const
{
	if (mWorld.has_value())
	{
		World world = std::move(*mWorld);
		mWorld.reset();

		if (!mSerializedWorld.has_value())
		{
			mSerializedWorld.emplace(Archiver::Serialize(world));
		}

		if (callBeginPlayImmediately)
		{
			world.BeginPlay();
		}

		return world;
	}

	World world{ false };

	if (mSerializedWorld.has_value())
	{
		Archiver::Deserialize(world, *mSerializedWorld);
	}
	else
	{
		LOG(LogAssets, Warning, "mWorld and mSerializedWorld were both null for {}", GetName());
	}

	if (callBeginPlayImmediately)
	{
		world.BeginPlay();
	}

	return world;
}

CE::World CE::Level::CreateDefaultWorld()
{
	World world{ false };
	Registry& reg = world.GetRegistry();

	{
		const entt::entity camera = reg.Create();
		reg.AddComponent<CameraComponent>(camera);
		reg.AddComponent<NameComponent>(camera, "Main Camera");

		TransformComponent& transform = reg.AddComponent<TransformComponent>(camera);
		transform.SetLocalPosition({ -10.0, 10.0f, 6.0f });
		transform.SetLocalOrientation(glm::quat{ glm::vec3{ 0.0f, glm::radians(22.5), glm::radians(-45.0f) } });
	}

	
	const glm::vec3 mainLightDir{ glm::radians(0.0f), glm::radians(38.5f), glm::radians(-51.0f) };

	{
		const entt::entity light = reg.Create();
		reg.AddComponent<NameComponent>(light, "Main Light");
		DirectionalLightComponent& lightComponent = reg.AddComponent<DirectionalLightComponent>(light);
		lightComponent.mColor *= .7f;

		reg.AddComponent<TransformComponent>(light).SetLocalOrientation(glm::quat{ mainLightDir });
	}

	{
		const entt::entity light = reg.Create();
		reg.AddComponent<NameComponent>(light, "Secondary Light");
		reg.AddComponent<DirectionalLightComponent>(light).mColor *= .3f;
		reg.AddComponent<TransformComponent>(light).SetLocalOrientation(glm::quat{ -mainLightDir });
	}

	return world;
}

std::vector<entt::entity> GetIds(const CE::BinaryGSONObject& serializedFactory)
{
	const CE::BinaryGSONMember* serializedIds = serializedFactory.TryGetGSONMember("IDS");

	std::vector<entt::entity> ids;
	if (serializedIds != nullptr)
	{
		*serializedIds >> ids;
	}
	return ids;
}

void CE::EraseSerializedComponents(World& world,
	const std::optional<std::string>& nameOfClassToErase,
	const std::vector<entt::entity>& eraseFromIds)
{
	if (!nameOfClassToErase.has_value())
	{
		world.GetRegistry().Destroy(eraseFromIds.begin(), eraseFromIds.end(), false);
		return;
	}

	const MetaType* typeToErase = MetaManager::Get().TryGetType(*nameOfClassToErase);

	if (typeToErase != nullptr)
	{
		for (const entt::entity entity : eraseFromIds)
		{
			world.GetRegistry().RemoveComponentIfEntityHasIt(typeToErase->GetTypeId(), entity);
		}
	}
}

void CE::EraseSerializedFactory(World& world,
	const BinaryGSONObject& serializedFactory,
	const std::string& prefabName)
{
	const Name::HashType hashedPrefabName = Name::HashString(prefabName);
	const uint32 factoryId = FromBinary<uint32>(serializedFactory.GetName());

	for (const auto [entity, prefabOrigin] : world.GetRegistry().View<PrefabOriginComponent>().each())
	{
		if (prefabOrigin.GetHashedPrefabName() == hashedPrefabName
			&& prefabOrigin.GetFactoryId() == factoryId)
		{
			world.GetRegistry().Destroy(entity, false);
		}
	}

	const auto children = GetSerializedChildFactories(serializedFactory);

	for (const BinaryGSONObject& serializedChildFactory : children)
	{
		EraseSerializedFactory(world, serializedChildFactory, prefabName);
	}
}

namespace CE
{
	BinaryGSONObject& GetSerializedPrefabFactory(BinaryGSONObject& prefabObject, const PrefabEntityFactory& prefabFactory);

	BinaryGSONObject& GetOrAddSerializedPrefabObject(BinaryGSONObject& prefabsObject, const Prefab& prefab);
}

CE::Level::DiffedPrefabFactory CE::Level::DiffPrefabFactory(const BinaryGSONObject& serializedFactory, 
	const PrefabEntityFactory& currentVersion)
{
	DiffedPrefabFactory returnValue{ currentVersion, serializedFactory };
	const std::vector<ComponentFactory>& currentComponents = currentVersion.GetComponentFactories();

	std::vector<std::string> componentNames{};

	if (const BinaryGSONMember* const serializedComponentNames = serializedFactory.TryGetGSONMember("Components");
		serializedComponentNames != nullptr)
	{
		*serializedComponentNames >> componentNames;

		// In case any of our components got renamed
		for (std::string& name : componentNames)
		{
			const MetaType* componentType = MetaManager::Get().TryGetType(name);

			if (componentType != nullptr)
			{
				name = componentType->GetName();
			}
		}
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

CE::Level::DiffedPrefab CE::Level::DiffPrefab(const BinaryGSONObject& serializedPrefab)
{
	const std::string& prefabName = serializedPrefab.GetName();

	DiffedPrefab returnValue{ serializedPrefab.GetName() };

	const AssetHandle<Prefab> prefab = AssetManager::Get().TryGetAsset<Prefab>(prefabName);
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
			[&serializedFactoryId, &serializedFactories, &factory](const uint32 serializedFactoryIndex)
			{
				// Check if the serialised prefab factory
				// was the root factory.
				if (serializedFactories[serializedFactoryIndex].TryGetGSONMember("ParentId") == nullptr)
				{
					// We don't care if the Ids match,
					// its possible the prefab got renamed;
					// because of my lack of fore-sight, the
					// root factory's id is based on the
					// prefab's name.
					return factory.GetParent() == nullptr;
				}

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

CE::BinaryGSONObject CE::Level::GenerateCurrentStateOfPrefabs(const BinaryGSONObject& serializedWorld)
{
	BinaryGSONObject prefabsObject{ "Prefabs" };

	struct SortPrefabs
	{
		bool operator()(const PrefabEntityFactory* a, const PrefabEntityFactory* b) const
		{
			if (a->GetPrefab().GetName() == b->GetPrefab().GetName())
			{
				return a->GetId() > b->GetId();
			}

			return a->GetPrefab().GetName() > b->GetPrefab().GetName();
		}
	};

	std::map<const PrefabEntityFactory*, std::vector<entt::entity>, SortPrefabs> idsPerFactory{};

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

		const Prefab* prefab = AssetManager::Get().TryGetAsset<Prefab>(hashedPrefabName).Get();

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

	for (auto& [factory, prefabIdsPair] : idsPerFactory)
	{
		BinaryGSONObject& serializedPrefab = GetOrAddSerializedPrefabObject(prefabsObject, factory->GetPrefab());
		BinaryGSONObject& serializedFactory = GetSerializedPrefabFactory(serializedPrefab, *factory);

		std::sort(prefabIdsPair.begin(), prefabIdsPair.end());

		serializedFactory.AddGSONMember("IDS") << prefabIdsPair;
	}

	return prefabsObject;
}

std::vector<entt::entity> CE::GetIds(const BinaryGSONObject& serializedFactory)
{
	const BinaryGSONMember* serializedIds = serializedFactory.TryGetGSONMember("IDS");

	std::vector<entt::entity> ids;
	if (serializedIds != nullptr)
	{
		*serializedIds >> ids;
	}
	return ids;
}

std::vector<std::reference_wrapper<const CE::BinaryGSONObject>> CE::GetSerializedChildFactories(const BinaryGSONObject& serializedFactory)
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

CE::BinaryGSONObject& CE::GetSerializedPrefabFactory(BinaryGSONObject& prefabObject, const PrefabEntityFactory& prefabFactory)
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

CE::BinaryGSONObject& CE::GetOrAddSerializedPrefabObject(BinaryGSONObject& prefabsObject, const Prefab& prefab)
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

CE::MetaType CE::Level::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Level>{}, "Level", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
	ReflectAssetType<Level>(type);
	return type;
}