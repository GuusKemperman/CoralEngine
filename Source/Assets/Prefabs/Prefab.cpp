#include "Precomp.h"
#include "Assets/Prefabs/Prefab.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Core/Editor.h"
#include "GSON/GSONBinary.h"
#include "World/Archiver.h"
#include "Components/TransformComponent.h"
#include "Components/PrefabOriginComponent.h"
#include "Utilities/Random.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"

Engine::Prefab::Prefab(std::string_view name) :
	Asset(name, MakeTypeId<Prefab>())
{
	World world{ false };
	CreateFromEntity(world, world.GetRegistry().Create());
}

Engine::Prefab::Prefab(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	BinaryGSONObject object{};
	object.LoadFromBinary(loadInfo.GetStream());
	LoadFromGSON(object);
}

Engine::Prefab::Prefab(Prefab&& other) noexcept :
	Asset(std::move(other)),
	mFactories(std::move(other.mFactories))
{
	for (PrefabEntityFactory& factory : mFactories)
	{
		factory.mPrefab = *this;
	}
}

Engine::Prefab::~Prefab() = default;

void Engine::Prefab::OnSave(AssetSaveInfo& saveInfo) const
{
	World world{ false };
	const entt::entity createdEntity = world.GetRegistry().CreateFromPrefab(*this);
	OnSave(saveInfo, GetName(), world, createdEntity, mFactoryIdSeed);
}

void Engine::Prefab::OnSave(AssetSaveInfo& saveInfo, const std::string& prefabName, World& world, const entt::entity entity, std::optional<uint32> factorySeed)
{
	const BinaryGSONObject& gsonObject = SaveToGSONObject(prefabName, world, entity, factorySeed);
	gsonObject.SaveToBinary(saveInfo.GetStream());
}

namespace Engine
{
	struct SerializedFactoryData
	{
		entt::entity mEntity = entt::null;
		uint32 mIndexOfParentFactory = std::numeric_limits<uint32>::max();
		uint32 mFactoryId{};
	};
}

namespace cereal
{
	static void save(BinaryOutputArchive& ar, const Engine::SerializedFactoryData& data)
	{
		ar(data.mEntity, data.mIndexOfParentFactory, data.mFactoryId);
	}

	static void load(BinaryInputArchive& ar, Engine::SerializedFactoryData& data)
	{
		ar(data.mEntity, data.mIndexOfParentFactory, data.mFactoryId);
	}
}

void Engine::Prefab::LoadFromGSON(BinaryGSONObject& object)
{
	mFactories.clear();

	std::vector<SerializedFactoryData> factoriesData{};

	{
		const BinaryGSONMember* const serializedFactories = object.TryGetGSONMember("factories");

		if (serializedFactories == nullptr)
		{
			LOG(Assets, Error, "Invalid prefab {}: Prefab is empty", GetName());
			return;
		}

		*serializedFactories >> factoriesData;
	}

	for (const SerializedFactoryData& data : factoriesData)
	{
		if (data.mIndexOfParentFactory != std::numeric_limits<uint32>::max()
			&& data.mIndexOfParentFactory >= factoriesData.size())
		{
			LOG(Assets, Error, "Invalid prefab {}: Index of parent factory is out of bounds", GetName());
			return;
		}
	}

	{
		const BinaryGSONMember* serializedSeed = object.TryGetGSONMember("seed");

		if (serializedSeed == nullptr)
		{
			LOG(Assets, Error, "Invalid prefab {}: Prefab is empty", GetName());
			return;
		}

		*serializedSeed >> mFactoryIdSeed;
	}

	// This guarantees that the vector does not need to
	// be resized, so passing in the parent/child pointers
	// is safe.
	mFactories.reserve(factoriesData.size());

	std::vector<std::pair<const PrefabEntityFactory*, std::vector<std::reference_wrapper<const PrefabEntityFactory>>>> parentChildRelations(factoriesData.size());

	for (size_t i = 0; i < factoriesData.size(); i++)
	{
		const SerializedFactoryData& factoryData = factoriesData[i];
		auto& parentChildRelation = parentChildRelations[i];

		parentChildRelation.first = factoryData.mIndexOfParentFactory == std::numeric_limits<uint32>::max() ? nullptr : mFactories.data() + factoryData.mIndexOfParentFactory;

		for (size_t j = 0; j < factoriesData.size(); j++)
		{
			if (factoriesData[j].mIndexOfParentFactory == i)
			{
				parentChildRelation.second.emplace_back(*(mFactories.data() + j));
			}
		}
	}

	for (size_t i = 0; i < factoriesData.size(); i++)
	{
		const SerializedFactoryData& factoryData = factoriesData[i];
		auto& parentChildRelation = parentChildRelations[i];
		mFactories.emplace_back(*this, object, factoryData.mEntity, factoryData.mFactoryId, parentChildRelation.first, std::move(parentChildRelation.second));
	}

}

Engine::BinaryGSONObject Engine::Prefab::SaveToGSONObject(const std::string& prefabName, World& world, const entt::entity rootEntity, std::optional<uint32> factorySeed)
{
	Registry& reg = world.GetRegistry();

	if (!reg.Valid(rootEntity))
	{
		LOG(LogAssets, Error, "Cannot create prefab from entity {}, this entity does not exists", static_cast<EntityType>(rootEntity));
		return {};
	}

	std::vector<std::pair<const entt::entity, const std::optional<PrefabOriginComponent>>> originsOfEachEntity{};

	// We only serialize the non-default values; in order to ingore the default values of a prefab (which may be an older version of this prefab),
	// and save all values instead, we reset the factories
	std::function<void(entt::entity)> recursivelyResetFactories =
		[&](entt::entity entityToReset)
		{
			if (const PrefabOriginComponent* origin = reg.TryGet<const PrefabOriginComponent>(entityToReset);
				origin != nullptr)
			{
				originsOfEachEntity.emplace_back(entityToReset, *origin);
				reg.RemoveComponent<PrefabOriginComponent>(entityToReset);
			}
			else
			{
				originsOfEachEntity.emplace_back(entityToReset, std::nullopt);
			}

			if (const TransformComponent* transform = reg.TryGet<TransformComponent>(entityToReset);
				transform != nullptr)
			{
				for (const TransformComponent& child : transform->GetChildren())
				{
					recursivelyResetFactories(child.GetOwner());
				}
			}
		};
	recursivelyResetFactories(rootEntity);

	std::vector<uint32> uniqueFactoryIds(originsOfEachEntity.size());
	const Name::HashType prefabHashedName = Name::HashString(prefabName);

	// The id of the first factory is always
	// predictable. If we create a prefab
	// from a different entity that was
	// not created using this prefab,
	// the factoryId would normally be generated
	// again. This makes it impossible to properly
	// find the changes made to a prefab when
	// deserializing a level.
	//
	// So while this line may seem insignificant,
	// please do not remove it :)
	uniqueFactoryIds[0] = prefabHashedName;

	for (size_t i = 1; i < originsOfEachEntity.size(); i++)
	{
		const auto& [entity, origin] = originsOfEachEntity[i];

		// This entity was not created by our prefab.
		// We will have to generate a new id in the
		// new for loop below.
		if (!origin.has_value()
			|| origin->GetHashedPrefabName() != prefabHashedName)
		{
			continue;
		}

		const uint32 factoryId = origin->GetFactoryId();

		auto searchEnd = uniqueFactoryIds.begin() + i;

		if (std::find(uniqueFactoryIds.begin(), searchEnd, factoryId) == searchEnd)
		{
			uniqueFactoryIds[i] = factoryId;
		}
	}

	uint32 seed = factorySeed.value_or(prefabHashedName);

	for (uint32& uniqueId : uniqueFactoryIds)
	{
		if (uniqueId != 0)
		{
			continue;
		}

		uint32 idCopy = uniqueId;

		do
		{
			idCopy = Random::Uint32(seed);
		} while (std::find(uniqueFactoryIds.begin(), uniqueFactoryIds.end(), idCopy) != uniqueFactoryIds.end());

		uniqueId = idCopy;
	}


	std::vector<SerializedFactoryData> factoriesData(originsOfEachEntity.size());

	for (size_t i = 0; i < originsOfEachEntity.size(); i++)
	{
		const auto& [entity, origin] = originsOfEachEntity[i];
		SerializedFactoryData& factoryData = factoriesData[i];
		factoryData.mFactoryId = uniqueFactoryIds[i];
		factoryData.mEntity = entity;

		if (const TransformComponent* transform = reg.TryGet<TransformComponent>(entity);
			transform != nullptr)
		{
			if (const TransformComponent* parent = transform->GetParent();
				parent != nullptr)
			{
				for (size_t j = 0; j < originsOfEachEntity.size(); j++)
				{
					if (parent->GetOwner() == originsOfEachEntity[j].first)
					{
						factoryData.mIndexOfParentFactory = static_cast<uint32>(j);
						break;
					}
				}
			}
		}
	}

	BinaryGSONObject returnValue = Archiver::Serialize(world, rootEntity, true);

	returnValue.AddGSONMember("factories") << factoriesData;
	returnValue.AddGSONMember("seed") << seed;

	for (size_t i = 0; i < originsOfEachEntity.size(); i++)
	{
		reg.AddComponent<PrefabOriginComponent>(originsOfEachEntity[i].first, PrefabOriginComponent{ prefabHashedName, uniqueFactoryIds[i] });
	}

	return returnValue;
}

void Engine::Prefab::CreateFromEntity(World& world, const entt::entity entity)
{
	BinaryGSONObject serializedPrefab = SaveToGSONObject(GetName(), world, entity);
	LoadFromGSON(serializedPrefab);
}

const Engine::PrefabEntityFactory* Engine::Prefab::TryFindFactory(uint32 factoryId) const
{
	const auto it = std::find_if(mFactories.begin(), mFactories.end(),
		[factoryId](const PrefabEntityFactory& factory)
		{
			return factory.GetId() == factoryId;
		});
	return it == mFactories.end() ? nullptr : &*it;
}

Engine::MetaType Engine::Prefab::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Prefab>{}, "Prefab", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
	ReflectAssetType<Prefab>(type);
	return type;
}