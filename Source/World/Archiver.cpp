#include "Precomp.h"
#include "World/Archiver.h"

#include "World/Registry.h"
#include "GSON/GSONBinary.h"
#include "Assets/Prefabs/ComponentFactory.h"
#include "Components/TransformComponent.h"
#include "Assets/Prefabs/PrefabEntityFactory.h"
#include "Components/PrefabOriginComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaFuncId.h"
#include "Meta/MetaProps.h"
#include "World/World.h"

namespace CE
{
	static void DeserializeStorage(Registry& registry, const BinaryGSONObject& serializedStorage, const std::unordered_map<entt::entity, entt::entity>& idRemappings);

	struct ComponentClassSerializeArg
	{
		const entt::sparse_set& mStorage;
		const MetaType& mComponentClass;
		MetaAny mComponentDefaultConstructed;
		std::vector<const MetaFunc*> mEqualityFunctions{};
		std::vector<const MetaFunc*> mSerializeMemberFunction{};
	};

	static std::optional<ComponentClassSerializeArg> GetComponentClassSerializeArg(const entt::sparse_set& storage);

	static void SerializeSingleComponent(const Registry& registry,
		BinaryGSONObject& parentObject,
		entt::entity entity,
		const ComponentClassSerializeArg& arg);
}

std::vector<entt::entity> CE::Archiver::Deserialize(World& world, const BinaryGSONObject& serializedWorld)
{
	Registry& reg = world.GetRegistry();

	const BinaryGSONMember* serializedEntities = serializedWorld.TryGetGSONMember("entities");

	if (serializedEntities == nullptr)
	{
		LOG(LogAssets, Error, "Invalid serialized world provided, this object was not created using Archiver::Serialize");
		return {};
	}

	std::vector<entt::entity> entities{};
	*serializedEntities >> entities;

	// In case an entity id is taken
	std::unordered_map<entt::entity, entt::entity> idRemappings{};
	idRemappings.reserve(entities.size());
	reg.Storage<entt::entity>().reserve(entities.size());

	for (entt::entity& entity : entities)
	{
		const entt::entity createdEntity = reg.Create(entity);
		idRemappings.emplace(entity, createdEntity);
		entity = createdEntity;
	}

	// We need to be able to retrieve each entity's prefab of origin while
	// deserializing, as it influences what is considered to be
	// a 'default' value.
	const BinaryGSONObject* serializedPrefabFactoryOfOrigin = serializedWorld.TryGetGSONObject(MetaManager::Get().GetType<PrefabOriginComponent>().GetName());
	if (serializedPrefabFactoryOfOrigin != nullptr)
	{
		DeserializeStorage(reg, *serializedPrefabFactoryOfOrigin, idRemappings);
	}

	for (const BinaryGSONObject& serializedStorage : serializedWorld.GetChildren())
	{
		if (&serializedStorage == serializedPrefabFactoryOfOrigin)
		{
			continue;
		}

		DeserializeStorage(reg, serializedStorage, idRemappings);
	}

	return entities;
}

void CE::DeserializeStorage(Registry& registry, const BinaryGSONObject& serializedStorage, const std::unordered_map<entt::entity, entt::entity>& idRemappings)
{
	const MetaType* const componentClass = MetaManager::Get().TryGetType(serializedStorage.GetName());

	if (componentClass == nullptr)
	{
		LOG(LogAssets, Verbose, "The class {} no longer exists and won't be deserialized.", serializedStorage.GetName());
		return;
	}

	if (componentClass->GetProperties().Has(Props::sNoSerializeTag))
	{
		return;
	}

	const bool checkForFactoryOfOrigin = componentClass->GetTypeId() != MakeTypeId<PrefabOriginComponent>();

	auto remapId = [&idRemappings](entt::entity entity) -> entt::entity
		{
			// Check if this is one of the many entities we are deserializing
			const auto it = idRemappings.find(entity);

			if (it != idRemappings.end())
			{
				// This entity is part of our deserializing process
				return it->second;
			}

			// Otherwise, leave it as it is.
			return entity;
		};

	const size_t storageInitialSize = registry.Storage(componentClass->GetTypeId()) == nullptr ? 0u : registry.Storage(componentClass->GetTypeId())->size();

	for (const BinaryGSONObject& serializedComponent : serializedStorage.GetChildren())
	{
		const std::vector<BinaryGSONMember>& serializedProperties = serializedComponent.GetGSONMembers();
		const entt::entity owner = remapId(FromBinary<entt::entity>(serializedComponent.GetName()));

		if (!registry.Valid(owner))
		{
			LOG(LogAssets, Error, "{} has owner {}, but this entity does not exist", componentClass->GetName(), entt::to_integral(owner));
			continue;
		}

		entt::sparse_set* storage = registry.Storage(componentClass->GetTypeId());
		if (storage != nullptr)
		{
			storage->reserve(storageInitialSize + serializedStorage.GetChildren().size());
		}

		MetaAny component{ *componentClass, nullptr };

		const ComponentFactory* componentFactoryOfOrigin{};

		if (checkForFactoryOfOrigin)
		{
			const PrefabOriginComponent* const prefabOrigin = registry.TryGet<const PrefabOriginComponent>(owner);

			if (prefabOrigin != nullptr)
			{
				const PrefabEntityFactory* const entityFactoryOfOrigin = prefabOrigin->TryGetFactory();

				if (entityFactoryOfOrigin != nullptr)
				{
					componentFactoryOfOrigin = entityFactoryOfOrigin->TryGetComponentFactory(*componentClass);
				}
			}
		}

		if (componentFactoryOfOrigin != nullptr)
		{
			component = componentFactoryOfOrigin->Construct(registry, owner);
		}
		else
		{
			component = registry.AddComponent(*componentClass, owner);
		}

		for (const BinaryGSONMember& serializedProperty : serializedProperties)
		{
			const MetaField* const field = componentClass->TryGetField(FromBinary<Name::HashType>(serializedProperty.GetName()));

			if (field == nullptr)
			{
				LOG(LogAssets, Verbose, "Could not find property whose name generated the hash {} while deserializing a component of class {}",
					serializedProperty.GetName(), componentClass->GetName());
				continue;
			}

			if (field->GetProperties().Has(Props::sNoSerializeTag))
			{
				continue;
			}

			const MetaType& memberType = field->GetType();

			MetaAny fieldValue = field->MakeRef(component);
			FuncResult deserializeMemberResult = memberType.CallFunction(sDeserializeMemberFuncName, serializedProperty, fieldValue);

			if (deserializeMemberResult.HasError())
			{
				LOG(LogAssets, Warning, "Could not deserialize {}::{} - {}",
					componentClass->GetName(),
					memberType.GetName(),
					deserializeMemberResult.Error());
			}
			else if (memberType.GetTypeId() == MakeTypeId<entt::entity>())
			{
				entt::entity& asEntity = *fieldValue.As<entt::entity>();
				asEntity = remapId(asEntity);
			}
		}
	}


	// TransformComponents are kinda dumb, back when i made them I added
	// pointer stability, and the parent/child relations are stored by
	// pointer and by entity id.
	// Sooo we have a beautiful hardcoded solution!
	if (componentClass->GetTypeId() != MakeTypeId<TransformComponent>())
	{
		return;
	}

	for (const BinaryGSONObject& serializedComponent : serializedStorage.GetChildren())
	{
		if (serializedComponent.GetChildren().empty())
		{
			continue;
		}

		const BinaryGSONObject& additionalSerializedData = serializedComponent.GetChildren()[0];
		const entt::entity owner = remapId(FromBinary<entt::entity>(serializedComponent.GetName()));

		TransformComponent* transform = registry.TryGet<TransformComponent>(owner);

		if (transform == nullptr
			|| additionalSerializedData.GetGSONMembers().size() != 1)
		{
			LOG(LogAssets, Warning, "Could not deserialize transform parent-child relation, invalid saved data");
			continue;
		}

		entt::entity parentEntity;
		additionalSerializedData.GetGSONMembers()[0] >> parentEntity;
		parentEntity = remapId(parentEntity);

		TransformComponent* parent = registry.TryGet<TransformComponent>(parentEntity);

		if (parent == nullptr)
		{
			LOG(LogAssets, Warning, "Could not deserialize transform parent-child relation, parent does not exist anymore");
		}

		transform->SetParent(parent);
	}
}

CE::BinaryGSONObject CE::Archiver::Serialize(const World& world)
{
	std::vector<entt::entity> entitiesToSerialize{};
	const auto* entityStorage = world.GetRegistry().Storage<entt::entity>();

	if (entityStorage != nullptr)
	{
		entitiesToSerialize.reserve(entityStorage->size());

		for (auto [entity] : entityStorage->each())
		{
			ASSERT(entityStorage->contains(entity));
			entitiesToSerialize.emplace_back(entity);
		}
	}

	return SerializeInternal(world, std::move(entitiesToSerialize), true);
}

CE::BinaryGSONObject CE::Archiver::Serialize(const World& world, Span<const entt::entity> entities, bool serializeChildren)
{
	std::vector<entt::entity> entitiesToSerialize{ entities.begin(), entities.end() };

	if (serializeChildren)
	{
		const Registry& reg = world.GetRegistry();

		for (const entt::entity entity : entities)
		{
			const TransformComponent* transform = reg.TryGet<TransformComponent>(entity);

			if (transform != nullptr)
			{
				std::function<void(const TransformComponent&)> addChildren = [&entitiesToSerialize, &addChildren](const TransformComponent& parent)
					{
						for (const TransformComponent& child : parent.GetChildren())
						{
							entitiesToSerialize.emplace_back(child.GetOwner());
							addChildren(child);
						}
					};
				addChildren(*transform);
			}
		}
	}

	// Remove any potential duplicates
	std::sort(entitiesToSerialize.begin(), entitiesToSerialize.end());
	entitiesToSerialize.erase(std::unique(entitiesToSerialize.begin(), entitiesToSerialize.end()), entitiesToSerialize.end());

	return SerializeInternal(world, std::move(entitiesToSerialize), false);
}

CE::BinaryGSONObject CE::Archiver::SerializeInternal(const World& world, std::vector<entt::entity>&& entitiesToSerialize,
                                                     bool allEntitiesInWorldAreBeingSerialized)
{
	const Registry& reg = world.GetRegistry();

	BinaryGSONObject save{ "SerializedWorld" };

	std::sort(entitiesToSerialize.begin(), entitiesToSerialize.end());

	save.AddGSONMember("entities") << entitiesToSerialize;

	size_t numOfStorages{};
	for ([[maybe_unused]] auto _ : reg.Storage())
	{
		++numOfStorages;
	}
	save.ReserveChildren(numOfStorages);

	for (auto&& [typeId, storage] : reg.Storage())
	{
		if (storage.empty())
		{
			continue;
		}

		const std::optional<ComponentClassSerializeArg> serializeArg = GetComponentClassSerializeArg(storage);

		if (!serializeArg.has_value())
		{
			continue;
		}

		BinaryGSONObject& serializedComponentClass = save.AddGSONObject(serializeArg->mComponentClass.GetName());
		serializedComponentClass.ReserveChildren(storage.size());

		for (const entt::entity entity : storage)
		{
			if (!allEntitiesInWorldAreBeingSerialized
				&& !std::binary_search(entitiesToSerialize.begin(), entitiesToSerialize.end(), entity))
			{
				continue;
			}

			SerializeSingleComponent(reg,
				serializedComponentClass,
				entity,
				*serializeArg);
		}

		// We want to guarantee that after deserializing this and then reserializing it, we get the same result.
		// But entt::registry will jumble up the order of the entities for us.
		// This ensures that we get the same order every time we save.
		std::sort(serializedComponentClass.GetChildren().begin(), serializedComponentClass.GetChildren().end(),
			[](const BinaryGSONObject& lhs, const BinaryGSONObject& rhs)
			{
				ASSERT(lhs.GetName().size() == sizeof(entt::entity));
				ASSERT(rhs.GetName().size() == sizeof(entt::entity));

				// Faster than string comparisons
				return *reinterpret_cast<const entt::entity*>(lhs.GetName().c_str()) < *reinterpret_cast<const entt::entity*>(rhs.GetName().c_str());
			});
	}

	return save;
}

std::optional<CE::ComponentClassSerializeArg> CE::GetComponentClassSerializeArg(const entt::sparse_set& storage)
{
	const MetaType* const componentClass = MetaManager::Get().TryGetType(storage.type().hash());

	if (componentClass == nullptr)
	{
		LOG(LogAssets, Warning, "Cannot serialize component of type {}, as it was not reflected", storage.type().name());
		return std::nullopt;
	}

	if (componentClass->GetProperties().Has(Props::sNoSerializeTag))
	{
		return std::nullopt;
	}

	std::vector<const MetaFunc*> equalityFunctions{};
	std::vector<const MetaFunc*> serializeMemberFunctions{};

	equalityFunctions.reserve(componentClass->GetDirectFields().size());
	serializeMemberFunctions.reserve(componentClass->GetDirectFields().size());

	for (const MetaField& memberData : componentClass->EachField())
	{
		equalityFunctions.push_back(nullptr);
		serializeMemberFunctions.push_back(nullptr);

		if (memberData.GetProperties().Has(Props::sNoSerializeTag))
		{
			continue;
		}

		const MetaType& memberType = memberData.GetType();

		{ // operator==
			const TypeTraits equalityParamType = TypeTraits{ memberType.GetTypeId(), TypeForm::ConstRef };

			const MetaFunc* const func = memberType.TryGetFunc(OperatorType::equal, MakeFuncId(MakeTypeTraits<bool>(), { equalityParamType, equalityParamType }));

			if (func == nullptr)
			{
				LOG(LogAssets, Warning, "No operator== function for {} in {}, Now even non-default values will be serialized",
					memberData.GetName(),
					componentClass->GetName());
			}

			equalityFunctions[equalityFunctions.size() - 1] = func;
		}

		{ // Serialize field
			const MetaFunc* serializeMemberFunc = memberType.TryGetFunc(sSerializeMemberFuncName);

			if (serializeMemberFunc == nullptr)
			{
				LOG(LogAssets, Warning, "No {} function for {} in {}, it won't be serialized",
					sSerializeMemberFuncName.StringView(),
					memberData.GetName(),
					componentClass->GetName());
			}

			serializeMemberFunctions[serializeMemberFunctions.size() - 1] = serializeMemberFunc;
		}
	}

	FuncResult defaultComponentResult = componentClass->Construct();
	MetaAny defaultComponent = defaultComponentResult.HasError() ? MetaAny{ *componentClass, nullptr } : std::move(defaultComponentResult.GetReturnValue());

	return ComponentClassSerializeArg{
		storage,
		*componentClass,
		std::move(defaultComponent),
		std::move(equalityFunctions),
		std::move(serializeMemberFunctions)
	};
}

void CE::SerializeSingleComponent(const Registry& registry,
	BinaryGSONObject& parentObject,
	const entt::entity entity,
	const ComponentClassSerializeArg& arg)
{
	if (!arg.mStorage.contains(entity)
		|| !registry.Valid(entity)) // This check is needed, .contains does not check if the entity is alive
	{
		return;
	}

	ASSERT(parentObject.GetName() == arg.mComponentClass.GetName());

	// Note that this might be nullptr if it's an empty component
	MetaAny component = MetaAny{ arg.mComponentClass, const_cast<void*>(arg.mStorage.value(entity)), false };
	BinaryGSONObject& serializedComponent = parentObject.AddGSONObject(ToBinary(entity));
	serializedComponent.ReserveMembers(arg.mComponentClass.GetDirectFields().size());

	const PrefabOriginComponent* prefabOriginComponent = registry.TryGet<PrefabOriginComponent>(entity);
	const PrefabEntityFactory* const entityFactoryOfOrigin = prefabOriginComponent == nullptr ? nullptr : prefabOriginComponent->TryGetFactory();
	const ComponentFactory* const factoryOfOrigin = entityFactoryOfOrigin == nullptr ? nullptr : entityFactoryOfOrigin->TryGetComponentFactory(arg.mComponentClass);

	auto writeValue = [&](const MetaField& data, const size_t indexOfData)
		{
			const MetaFunc* const serializeMemberFunc = arg.mSerializeMemberFunction[indexOfData];

			if (serializeMemberFunc == nullptr)
			{
				return;
			}

			// We only save the hash, which makes the save significantly smaller
			const std::string hashedPropertyNameAsBinaryString = ToBinary(Name::HashString(data.GetName()));

			BinaryGSONMember& nonDefaultProperty = serializedComponent.AddGSONMember(hashedPropertyNameAsBinaryString);
			MetaAny memberRef = data.MakeRef(component);

			[[maybe_unused]] FuncResult result = (*serializeMemberFunc)(nonDefaultProperty, memberRef);
			ASSERT_LOG(!result.HasError(), "{}", result.Error());
		};

	// We only serialize the differences. Makes the save file smaller and any changes 
	// made to the default value propegate to components when deserializing.

	const auto eachMember = arg.mComponentClass.EachField();

	size_t i = 0;
	for (auto it = eachMember.begin(); it != eachMember.end(); ++it, ++i)
	{
		const MetaField& field = *it;

		if (field.GetProperties().Has(Props::sNoSerializeTag))
		{
			continue;
		}

		const MetaFunc* const equalityOperator = arg.mEqualityFunctions[i];

		// We only serialize it if it's different from the default value.
		// Unfortunately, we cannot check for that if there's no equality operator
		if (equalityOperator == nullptr)
		{
			writeValue(field, i);
			continue;
		}

		MetaAny valueInComponent = field.MakeRef(component);

		FuncResult result;

		// Check to see if this object was created by a prefab who has set a different default value for this field
		const MetaAny* const valueAsOverridenByPrefabOfOrigin = factoryOfOrigin == nullptr ?
			nullptr :
			factoryOfOrigin->GetOverridenDefaultValue(field);

		if (valueAsOverridenByPrefabOfOrigin != nullptr)
		{
			result = (*equalityOperator)(valueInComponent, *valueAsOverridenByPrefabOfOrigin);
		}
		else if (arg.mComponentDefaultConstructed != nullptr)
		{
			MetaAny defaultvalue = field.MakeRef(const_cast<MetaAny&>(arg.mComponentDefaultConstructed));
			result = (*equalityOperator)(valueInComponent, defaultvalue);
		}
		else
		{
			// No default component to compare it to, so we'll skip it
			writeValue(field, i);
			continue;
		}

		ASSERT(!result.HasError());
		ASSERT(result.HasReturnValue());

		const bool* areEqual = result.GetReturnValue().As<bool>();
		ASSERT(areEqual != nullptr);

		if (!*areEqual)
		{
			writeValue(field, i);
		}
	}

	if (arg.mComponentClass.GetTypeId() != MakeTypeId<TransformComponent>())
	{
		return;
	}

	const TransformComponent& transform = registry.Get<TransformComponent>(entity);

	if (transform.GetParent() != nullptr)
	{
		// Since the parent/children are stored through a raw ptr they
		// are trickier to serialize. Since this is the only place where
		// storing a raw pointer to a component makes sense, we're not going
		// to bother with making a system that allows serializing component pointers
		serializedComponent.AddGSONObject("").AddGSONMember("") << transform.GetParent()->GetOwner();
	}
}