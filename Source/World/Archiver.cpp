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

namespace Engine
{
	static void DeserializeStorage(Registry& registry, const BinaryGSONObject& serializedStorage);

	struct ComponentClassSerializeArg
	{
		const entt::sparse_set& mStorage;
		const MetaType& mComponentClass;
		const MetaFunc* mCustomStep;
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

void Engine::Archiver::Deserialize(World& world, const BinaryGSONObject& serializedWorld)
{
	Registry& reg = world.GetRegistry();

	const BinaryGSONMember* serializedEntities = serializedWorld.TryGetGSONMember("entities");

	if (serializedEntities == nullptr)
	{
		LOG(LogAssets, Error, "Invalid serialized world provided, this object was not created using Archiver::Serialize");
		return;
	}

	std::vector<entt::entity> entities{};
	*serializedEntities >> entities;

	for (entt::entity& entity : entities)
	{
		entt::entity createdEntity = reg.Create(entity);

		if (createdEntity != entity)
		{
			LOG(LogAssets, Error, "Attempted to deserialize entity with id {}, but this entity could not be added to the registry. May lead to unexpected results.", static_cast<EntityType>(entity));
		}
	}

	// We need to be able to retrieve each entity's prefab of origin while
	// deserializing, as it influences what is considered to be
	// a 'default' value.
	const BinaryGSONObject* serializedPrefabFactoryOfOrigin = serializedWorld.TryGetGSONObject(MetaManager::Get().GetType<PrefabOriginComponent>().GetName());
	if (serializedPrefabFactoryOfOrigin != nullptr)
	{
		DeserializeStorage(reg, *serializedPrefabFactoryOfOrigin);
	}

	for (const BinaryGSONObject& serializedStorage : serializedWorld.GetChildren())
	{
		if (&serializedStorage == serializedPrefabFactoryOfOrigin)
		{
			continue;
		}

		DeserializeStorage(reg, serializedStorage);
	}
}

void Engine::DeserializeStorage(Registry& registry, const BinaryGSONObject& serializedStorage)
{
	const MetaType* const componentClass = MetaManager::Get().TryGetType(serializedStorage.GetName());

	if (componentClass == nullptr)
	{
		LOG(LogAssets, Warning, "The class {} no longer exists and won't be deserialized.", serializedStorage.GetName());
		return;
	}

	const bool checkForFactoryOfOrigin = componentClass->GetTypeId() != MakeTypeId<PrefabOriginComponent>();

	for (const BinaryGSONObject& serializedComponent : serializedStorage.GetChildren())
	{
		const std::vector<BinaryGSONMember>& serializedProperties = serializedComponent.GetGSONMembers();

		const entt::entity owner = FromBinary<entt::entity>(serializedComponent.GetName());

		if (!registry.Valid(owner))
		{
			LOG(LogAssets, Error, "Component has owner {}, but this entity does not exist", static_cast<EntityType>(owner));
			continue;
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
				LOG(LogAssets, Warning, "Could not find property whose name generated the hash {} while deserializing a component of class {}",
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
		}
	}

	// Call the custom deserialisation step only after all components have been created.
	// The custom deserialization is sometimes used when we want to serialize pointers to
	// components, the transformcomponent for example serialized it's parent as an entity 
	// id instead. This can't be deserialized properly into a pointer if that transform 
	// does not exist yet.
	auto* storage = registry.Storage(componentClass->GetTypeId());

	if (storage == nullptr)
	{
		return;
	}

	const MetaFunc* const onComponentDeserialize = TryGetEvent(*componentClass, sDeserializeEvent);

	if (onComponentDeserialize == nullptr)
	{
		return;
	}

	MetaAny worldRef{ registry.GetWorld() };

	for (const BinaryGSONObject& serializedComponent : serializedStorage.GetChildren())
	{
		if (serializedComponent.GetChildren().empty())
		{
			continue;
		}

		const BinaryGSONObject& additionalSerializedData = serializedComponent.GetChildren()[0];
		const entt::entity owner = FromBinary<entt::entity>(serializedComponent.GetName());

		ASSERT_LOG(storage->contains(owner), "Should've been created already");
		MetaAny componentRef{ *componentClass, storage->value(owner), false };

		FuncResult result = (*onComponentDeserialize)(componentRef, additionalSerializedData, owner, registry.GetWorld());

		if (result.HasError())
		{
			LOG(LogWorld, Error, "Error occured while calling custom deserialization step of {} - {}", componentClass->GetName(), result.Error());
		}
	}
}

Engine::BinaryGSONObject Engine::Archiver::Serialize(const World& world)
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

	return Serialize(world, std::move(entitiesToSerialize), true);
}

Engine::BinaryGSONObject Engine::Archiver::Serialize(const World& world, entt::entity entity, bool serializeChildren)
{
	std::vector<entt::entity> entitiesToSerialize{ entity };

	if (serializeChildren)
	{
		const TransformComponent* transform = world.GetRegistry().TryGet<TransformComponent>(entity);

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

	return Serialize(world, std::move(entitiesToSerialize), false);
}

Engine::BinaryGSONObject Engine::Archiver::Serialize(const World& world, std::vector<entt::entity> entitiesToSerialize,
                                                     bool allEntitiesInWorldAreBeingSerialized)
{
	const Registry& reg = world.GetRegistry();

	BinaryGSONObject save{ "SerializedWorld" };

	std::sort(entitiesToSerialize.begin(), entitiesToSerialize.end());

	save.AddGSONMember("entities") << entitiesToSerialize;

	for (auto&& [typeId, storage] : reg.Storage())
	{
		const std::optional<ComponentClassSerializeArg> serializeArg = GetComponentClassSerializeArg(storage);

		if (!serializeArg.has_value())
		{
			continue;
		}

		BinaryGSONObject& serializedComponentClass = save.AddGSONObject(serializeArg->mComponentClass.GetName());

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
				ASSERT(lhs.GetName().size() == sizeof(EntityType));
				ASSERT(rhs.GetName().size() == sizeof(EntityType));

				// Faster than string comparisons
				return *reinterpret_cast<const EntityType*>(lhs.GetName().c_str()) < *reinterpret_cast<const EntityType*>(rhs.GetName().c_str());
			});
	}

	return save;
}

std::optional<Engine::ComponentClassSerializeArg> Engine::GetComponentClassSerializeArg(const entt::sparse_set& storage)
{
	const MetaType* const componentClass = MetaManager::Get().TryGetType(storage.type().hash());

	if (componentClass == nullptr)
	{
		LOG(LogAssets, Warning, "Cannot serialize component of type {}, as it was not reflected", storage.type().name())
			return std::nullopt;
	}

	const MetaFunc* onSerialize = TryGetEvent(*componentClass, sSerializeEvent);
	std::vector<const MetaFunc*> equalityFunctions{};
	std::vector<const MetaFunc*> serializeMemberFunctions{};

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
		onSerialize,
		std::move(defaultComponent),
		std::move(equalityFunctions),
		std::move(serializeMemberFunctions)
	};
}

void Engine::SerializeSingleComponent(const Registry& registry,
	BinaryGSONObject& parentObject,
	const entt::entity entity,
	const ComponentClassSerializeArg& arg)
{
	if (!arg.mStorage.contains(entity))
	{
		return;
	}

	ASSERT(parentObject.GetName() == arg.mComponentClass.GetName());

	MetaAny component = MetaAny{ arg.mComponentClass, const_cast<void*>(arg.mStorage.value(entity)), false };
	BinaryGSONObject& serializedComponent = parentObject.AddGSONObject(ToBinary(entity));

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

	if (arg.mCustomStep != nullptr)
	{
		BinaryGSONObject& customStepObject = serializedComponent.AddGSONObject("");

		FuncResult result = (*arg.mCustomStep)(component, customStepObject, entity, registry.GetWorld());

		if (customStepObject.IsEmpty()
			|| result.HasError())
		{
			serializedComponent.GetChildren().pop_back();
		}

		if (result.HasError())
		{
			LOG(LogAssets, Error, "Could not invoke custom serialization step of {} - {}",
				arg.mComponentClass.GetName(),
				result.Error());
		}
	}
}