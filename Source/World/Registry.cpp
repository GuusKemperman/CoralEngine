#include "Precomp.h"
#include "World/Registry.h"

#include <entt/entity/runtime_view.hpp>

#include "Assets/Script.h"
#include "World/World.h"
#include "Assets/Prefabs/ComponentFactory.h"
#include "Assets/Prefabs/Prefab.h"
#include "Assets/Prefabs/PrefabEntityFactory.h"
#include "Components/CameraComponent.h"
#include "Components/IsDestroyedTag.h"
#include "Components/PrefabOriginComponent.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaAny.h"
#include "Meta/MetaTools.h"
#include "Scripting/ScriptTools.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/EventManager.h"

namespace CE::Internal
{
	class AnyStorage final :
		public entt::basic_sparse_set<>
	{
	public:
		using BaseType = basic_sparse_set<>;

		AnyStorage(const MetaType& type);
		~AnyStorage() override;

		static bool CanTypeBeUsed(const MetaType& type);

		const MetaType& GetType() const { return mType; }
		const MetaFunc* GetOnConstruct() const { return mOnConstruct; }
		const MetaFunc* GetOnBeginPlay() const { return mOnBeginPlay; }

		const void* get_at(std::size_t pos) const override;

		void* get_at(std::size_t pos);

		MetaAny element_at(size_t pos);

		void swap_or_move(std::size_t from, std::size_t to) override;

		void pop(basic_iterator first, basic_iterator last) override;

		void pop_all() override;

		basic_iterator try_emplace(entt::entity entt, bool force_back, const void* value) override;

		void reserve(size_type cap) override;

		[[nodiscard]] size_type capacity() const noexcept override
		{
			return mCapacity;
		}

		void shrink_to_fit() override {}

		const entt::type_info& GetTypeInfo() const { return mTypeInfo; }

	private:
		void reserve_atleast(size_type cap);

		std::reference_wrapper<const MetaType> mType;
		const MetaFunc* mOnConstruct{};
		const MetaFunc* mOnBeginPlay{};

		char* mData{};
		size_t mCapacity{};
		entt::type_info mTypeInfo;
	};

	class IsAwaitingBeginPlayTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
	};

	class IsAwaitingEndPlayTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
	};
}

CE::Registry::Registry(World& world) :
	mWorld(world)
{
	const MetaType* const systemType = MetaManager::Get().TryGetType<System>();
	ASSERT(systemType != nullptr);
	std::function<void(const MetaType&)> registerChildren =
		[&](const MetaType& type)
		{
			for (const MetaType& child : type.GetDirectDerivedClasses())
			{
				FuncResult childConstructResult = child.Construct();

				if (childConstructResult.HasError())
				{
					LOG(LogWorld, Error, "System {} is not default constructible - {}", child.GetName(), childConstructResult.Error());
					continue;
				}
				auto newSystem = MakeUnique<System>(std::move(childConstructResult.GetReturnValue()));

				AddSystem(std::move(newSystem));
				
				registerChildren(child);
			}
		};
	registerChildren(*systemType);
}

void CE::Registry::BeginPlay()
{
	for (const auto [entity] : Storage<entt::entity>().each())
	{
		if (!HasComponent<Internal::IsAwaitingBeginPlayTag>(entity))
		{
			AddComponent<Internal::IsAwaitingBeginPlayTag>(entity);
		}
	}

	CallBeginPlayForEntitiesAwaitingBeginPlay();
}

void CE::Registry::UpdateSystems(float dt)
{
	const std::vector<SingleTick> ticksToCall = GetSortedSystemsToUpdate(dt);
	World& world = GetWorld();

	for (const SingleTick& tick : ticksToCall)
	{
		tick.mSystem.get().Update(world, tick.mDeltaTime);
	}
}

void CE::Registry::RenderSystems() const
{
	if (CameraComponent::GetSelected(mWorld) == entt::null)
	{
		LOG(LogTemp, Message, "No camera to render to");
		return;
	}

	for (const FixedTickSystem& fixedTickSystem : mFixedTickSystems)
	{
		fixedTickSystem.mSystem->Render(mWorld);
	}

	for (const InternalSystem& internalSystem : mNonFixedSystems)
	{
		internalSystem.mSystem->Render(mWorld);
	}
}

entt::entity CE::Registry::Create()
{
	return mRegistry.create();
}

entt::entity CE::Registry::Create(entt::entity hint)
{
	return mRegistry.create(hint);
}

entt::entity CE::Registry::CreateFromPrefab(const Prefab& prefab, 
	entt::entity hint,
	const glm::vec3* localPosition,
	const glm::quat* localOrientation,
	const glm::vec3* localScale,
	TransformComponent* parent)
{
	const std::vector<PrefabEntityFactory>& factories = prefab.GetFactories();

	if (factories.empty())
	{
		UNLIKELY;
		LOG(LogAssets, Error, "Invalid prefab provided, the prefab {} is empty", prefab.GetName());
		return Create();
	}

	const entt::entity entity = CreateFromFactory(factories[0], true, hint);
	TransformComponent* transform = TryGet<TransformComponent>(entity);

	if (transform != nullptr)
	{
		if (localPosition != nullptr
			|| localOrientation != nullptr
			|| localScale != nullptr
			|| parent != nullptr)
		{

			if (localPosition != nullptr)
			{
				transform->SetLocalPosition(*localPosition);
			}

			if (localOrientation != nullptr)
			{
				transform->SetLocalOrientation(*localOrientation);
			}

			if (localScale != nullptr)
			{
				transform->SetLocalScale(*localScale);
			}

			if (parent != nullptr)
			{
				transform->SetParent(parent, false);
			}
		}

		transform->UpdateCachedWorldMatrix();
	}

	// We wait with calling BeginPlay until all the
	// components and children have been constructed.
	CallBeginPlayForEntitiesAwaitingBeginPlay();

	return entity;
}

entt::entity CE::Registry::CreateFromFactory(const PrefabEntityFactory& factory, bool createChildren, entt::entity hint)
{
	const entt::entity entity = Create(hint);

	// We wait with calling BeginPlay until all the
	// components and children have been constructed.
	AddComponent<Internal::IsAwaitingBeginPlayTag>(entity);

	AddComponent<PrefabOriginComponent>(entity).SetFactoryOfOrigin(factory);

	for (const ComponentFactory& componentFactory : factory.GetComponentFactories())
	{
		componentFactory.Construct(*this, entity);
	}

	if (!createChildren
		|| factory.GetChildren().empty())
	{
		return entity;
	}

	TransformComponent* const myTransform = TryGet<TransformComponent>(entity);

	if (myTransform == nullptr)
	{
		LOG(LogAssets, Error, "Invalid prefab provided, the prefab {} contains a parental relationship, but is missing certain transformcomponents", factory.GetPrefab().GetName());
		return entity;
	}

	for (const PrefabEntityFactory& childFactory : factory.GetChildren())
	{
		const entt::entity child = CreateFromFactory(childFactory, true);

		TransformComponent* const childTransform = TryGet<TransformComponent>(child);

		if (childTransform != nullptr)
		{
			LIKELY;
			childTransform->SetParent(myTransform);
		}
		else
		{
			LOG(LogAssets, Error, "Invalid prefab provided, the prefab {} contains a parental relationship, but is missing certain transformcomponents", factory.GetPrefab().GetName());
		}
	}
	
	return entity;
}

void CE::Registry::Destroy(entt::entity entity, bool destroyChildren)
{
	if (!Valid(entity) 
		|| HasComponent<IsDestroyedTag>(entity))
	{
		return;
	}

	AddComponent<IsDestroyedTag>(entity);

	if (destroyChildren)
	{
		TransformComponent* transform = TryGet<TransformComponent>(entity);

		if (transform == nullptr)
		{
			return;
		}

		std::function<void(TransformComponent&)> addToDestroyed = [&](TransformComponent& current)
			{
				for (TransformComponent& child : current.GetChildren())
				{
					addToDestroyed(child);
					Destroy(child.GetOwner(), true);
				}
			};
		addToDestroyed(*transform);
	}
}

void CE::Registry::RemovedDestroyed()
{
	World::PushWorld(mWorld);

	while (true)
	{
		{
			const auto isDestroyedView = View<const IsDestroyedTag>();

			if (isDestroyedView.empty())
			{
				break;
			}

			if (!mWorld.get().HasBegunPlay())
			{
				for (const entt::entity entity : isDestroyedView)
				{
					mRegistry.destroy(entity);
				}
				continue;
			}

			AddComponents<Internal::IsAwaitingEndPlayTag>(isDestroyedView.begin(), isDestroyedView.end());
		}

		entt::sparse_set& awaitingEndPlayStorage = Storage<Internal::IsAwaitingEndPlayTag>();

		// Dear bug hunter,
		// This loop is nice for DOD purposes,
		// but it does mean we are making a slight sacrifice;
		// if someone calls RemoveComponent for some reason on
		// an entity that we are currently destroying and one that
		// we have already processed, the OnEndPlay event for
		// that component will be called twice.
		// This was done consciously as the chances of it
		// occuring are tremendously low, and because using this
		// approach is significantly faster.
		for (const BoundEvent& endPlay : GetWorld().GetEventManager().GetBoundEvents(sOnEndPlay))
		{
			entt::sparse_set* componentStorage = Storage(endPlay.mType.get().GetTypeId());

			if (componentStorage == nullptr)
			{
				continue;
			}

			entt::runtime_view view{};
			view.iterate(awaitingEndPlayStorage);
			view.iterate(*componentStorage);

			for (const entt::entity entity : view)
			{
				if (endPlay.mIsStatic)
				{
					endPlay.mFunc.get().InvokeUncheckedUnpacked(GetWorld(), entity);
				}
				else
				{
					MetaAny component{ endPlay.mType, componentStorage->value(entity), false };
					endPlay.mFunc.get().InvokeUncheckedUnpacked(component, GetWorld(), entity);
				}
			}
		}

		// Now that we are sure all the EndPlay events have been called,
		// we actually permanently destroy the entities.
		const auto toPermanentlyDestroyView = View<Internal::IsAwaitingEndPlayTag>();

		for (const entt::entity entity : toPermanentlyDestroyView)
		{
			mRegistry.destroy(entity);
		}
	}

	World::PopWorld();
}

CE::MetaAny CE::Registry::AddComponent(const MetaType& componentClass, const entt::entity toEntity)
{
	if (WasTypeCreatedByScript(componentClass))
	{
		entt::sparse_set* storage = Storage(componentClass.GetTypeId());

		if (storage == nullptr)
		{
			if (!Internal::AnyStorage::CanTypeBeUsed(componentClass))
			{
				LOG(LogWorld, Error, "Failed to add component {} to {} - This class was created through scripts, and because of a programmer error it does not have all the functionality a component needs. My bad!",
					componentClass.GetName(),
					entt::to_integral(toEntity));
				return { componentClass, nullptr, false };
			}

			storage = &mRegistry.GuusEngineAddPool<Internal::AnyStorage>(componentClass);
			ASSERT(storage != nullptr);
		}
		ASSERT(dynamic_cast<Internal::AnyStorage*>(storage) != nullptr);

		storage = Storage(componentClass.GetTypeId());

		Internal::AnyStorage* const asAnyStorage = static_cast<Internal::AnyStorage*>(storage);
		const auto it = asAnyStorage->try_emplace(toEntity, false, nullptr);

		MetaAny componentToReturn = asAnyStorage->element_at(it.index());

		const MetaField* const ownerMember = componentClass.TryGetField(Script::sNameOfOwnerField);

		if (ownerMember != nullptr)
		{
			if (ownerMember->GetType().GetTypeId() == MakeTypeId<entt::entity>())
			{
				MetaAny refToMember = ownerMember->MakeRef(componentToReturn);
				*refToMember.As<entt::entity>() = toEntity;
			}
			else
			{
				LOG(LogScripting, Error, "Expected {}::Owner to be of type entt::entity",
					componentClass.GetName());
			}
		}

		// Call events
		const MetaFunc* const onConstruct = asAnyStorage->GetOnConstruct();

		if (onConstruct != nullptr)
		{
			onConstruct->InvokeUncheckedUnpacked(componentToReturn, GetWorld(), toEntity);
		}
		
		const MetaFunc* const onBeginPlay = asAnyStorage->GetOnBeginPlay();

		if (onBeginPlay != nullptr
			&& ShouldWeCallBeginPlayImmediatelyAfterConstruct(toEntity))
		{
			onBeginPlay->InvokeUncheckedUnpacked(componentToReturn, GetWorld(), toEntity);
		}

		return componentToReturn;
	}

	const MetaType& entityType = MetaManager::Get().GetType<entt::entity>();

	const std::string addComponentFuncName = Internal::GetAddComponentFuncName(componentClass.GetName());
	const MetaFunc* const addComponentFunc = entityType.TryGetFunc(addComponentFuncName);

	if (addComponentFunc == nullptr)
	{
		LOG(LogWorld, Error, "Failed to add component {} to {}: ReflectComponentType was never called for this type",
			componentClass.GetName(),
			entt::to_integral(toEntity));
		return MetaAny{ componentClass.GetTypeInfo(), nullptr };
	}

	World::PushWorld(mWorld);

	FuncResult result = addComponentFunc->InvokeUncheckedUnpacked(toEntity);

	World::PopWorld();

	if (result.HasError())
	{
		LOG(LogWorld, Error, "Failed to add component {} to {}: Invoking {} failed - {}",
			componentClass.GetName(),
			entt::to_integral(toEntity),
			addComponentFuncName,
			result.Error());
		return MetaAny{ componentClass.GetTypeInfo(), nullptr };
	}

	if (result.HasReturnValue())
	{
		return std::move(result.GetReturnValue());
	}
	return MetaAny{ componentClass.GetTypeInfo(), nullptr };
}

void CE::Registry::RemoveComponent(const TypeId componentClassTypeId, const entt::entity fromEntity)
{
	World::PushWorld(mWorld);
	entt::sparse_set* storage = Storage(componentClassTypeId);
	ASSERT(storage != nullptr);
	ASSERT(storage->contains(fromEntity));

	if (mWorld.get().HasBegunPlay())
	{
		const MetaType* type = MetaManager::Get().TryGetType(componentClassTypeId);

		if (type != nullptr)
		{
			const std::optional<BoundEvent> endPlayEvent = TryGetEvent(*type, sOnEndPlay);

			if (endPlayEvent.has_value())
			{
				CallEndPlayEventsForEntity(*storage, fromEntity, *endPlayEvent);
			}
		}
	}

	storage->erase(fromEntity);
	World::PopWorld();
}

void CE::Registry::RemoveComponentIfEntityHasIt(const TypeId componentClassTypeId, const entt::entity fromEntity)
{
	entt::sparse_set* storage = Storage(componentClassTypeId);

	if (storage == nullptr)
	{
		return;
	}

	World::PushWorld(mWorld);

	if (mWorld.get().HasBegunPlay())
	{
		if (!storage->contains(fromEntity))
		{
			World::PopWorld();
			return;
		}

		const MetaType* type = MetaManager::Get().TryGetType(componentClassTypeId);

		if (type != nullptr)
		{
			const std::optional<BoundEvent> endPlayEvent = TryGetEvent(*type, sOnEndPlay);

			if (endPlayEvent.has_value())
			{
				CallEndPlayEventsForEntity(*storage, fromEntity, *endPlayEvent);
			}
		}
	}

	storage->remove(fromEntity);
	World::PopWorld();
}

std::vector<CE::MetaAny> CE::Registry::GetComponents(entt::entity entity)
{
	std::vector<MetaAny> found{};

	for (auto&& [typeHash, storage] : Storage())
	{
		if (!storage.contains(entity))
		{
			continue;
		}

		found.emplace_back(storage.type().ToGuusTypeInfo<TypeInfo>(), storage.value(entity));
	}

	return found;
}

CE::MetaAny CE::Registry::TryGet(const TypeId componentClassTypeId, entt::entity entity)
{
	const auto storage = Storage(componentClassTypeId);

	if (storage != nullptr
		&& storage->contains(entity))
	{
		return MetaAny{ storage->type().ToGuusTypeInfo<TypeInfo>(), storage->value(entity) };
	}
	return MetaAny{ MakeTypeInfo<void>(), nullptr };
}

CE::MetaAny CE::Registry::Get(const TypeId componentClassTypeId, entt::entity entity)
{
	const auto storage = Storage(componentClassTypeId);
	ASSERT(storage != nullptr && storage->contains(entity));
	return MetaAny{ storage->type().ToGuusTypeInfo<TypeInfo>(), storage->value(entity) };
}

bool CE::Registry::HasComponent(TypeId componentClassTypeId, entt::entity entity) const
{
	const auto storage = Storage(componentClassTypeId);
	return storage != nullptr && storage->contains(entity);
}

void CE::Registry::Clear()
{
	World::PushWorld(mWorld);

	for (const entt::entity entity : mRegistry.storage<entt::entity>())
	{
		if (!HasComponent<IsDestroyedTag>(entity))
		{
			AddComponent<IsDestroyedTag>(entity);
		}
	}
	RemovedDestroyed();
	World::PopWorld();
}

std::vector<CE::Registry::SingleTick> CE::Registry::GetSortedSystemsToUpdate(const float dt)
{
	std::vector<SingleTick> returnValue{};

	const bool hasBegunPlay = GetWorld().HasBegunPlay();
	const bool isPaused = GetWorld().IsPaused();

	const auto canEverTick = [&](const SystemStaticTraits& traits)
		{
			if (hasBegunPlay)
			{
				return !isPaused || traits.mShouldTickWhilstPaused;
			}
			return traits.mShouldTickBeforeBeginPlay;
		};
	{
		struct SortableFixedTick
		{
			SortableFixedTick(FixedTickSystem& system, float timeOfNextStep) : mFixedTicksystem(system), mTimeOfNextStep(timeOfNextStep) {}
			std::reference_wrapper<FixedTickSystem> mFixedTicksystem;
			float mTimeOfNextStep{};
		};
		std::vector<SortableFixedTick> ticksToSort{};

		for (FixedTickSystem& fixedSystem : mFixedTickSystems)
		{
			if (!canEverTick(fixedSystem.mTraits))
			{
				continue;
			}

			float timeOfNextStep = fixedSystem.mTimeOfNextStep;

			while (timeOfNextStep <= (fixedSystem.mTraits.mShouldTickWhilstPaused ? 
				mWorld.get().GetCurrentTimeReal() : 
				mWorld.get().GetCurrentTimeScaled()))
			{
				ticksToSort.emplace_back(fixedSystem, timeOfNextStep);
				timeOfNextStep += *fixedSystem.mTraits.mFixedTickInterval;
			}

			fixedSystem.mTimeOfNextStep = timeOfNextStep;
		}

		std::sort(ticksToSort.begin(), ticksToSort.end(),
			[](const SortableFixedTick& l, const SortableFixedTick& r)
			{
				if (l.mTimeOfNextStep == r.mTimeOfNextStep)
				{
					return l.mFixedTicksystem.get().mTraits.mPriority > r.mFixedTicksystem.get().mTraits.mPriority;
				}
				else
				{
					return l.mTimeOfNextStep < r.mTimeOfNextStep;
				}
			});

		for (const SortableFixedTick& sortableFixedTick : ticksToSort)
		{
			returnValue.emplace_back(*sortableFixedTick.mFixedTicksystem.get().mSystem, *sortableFixedTick.mFixedTicksystem.get().mTraits.mFixedTickInterval);
		}
	}

	for (InternalSystem& internalSystem : mNonFixedSystems)
	{
		if (canEverTick(internalSystem.mTraits))
		{
			returnValue.emplace_back(*internalSystem.mSystem, dt);
		}
	}

	return returnValue;
}

void CE::Registry::AddSystem(std::unique_ptr<System, InPlaceDeleter<System, true>> system)
{
	SystemStaticTraits staticTraits = system->GetStaticTraits();

	if (staticTraits.mFixedTickInterval.has_value())
	{
		mFixedTickSystems.emplace_back(std::move(system), staticTraits, GetWorld().GetCurrentTimeScaled());
	}
	else
	{
		InternalSystem newInternal{ std::move(system), staticTraits };

		const auto whereToInsert = std::lower_bound(mNonFixedSystems.begin(), mNonFixedSystems.end(), newInternal,
		                                            [](const InternalSystem& sl, const InternalSystem& sr)
		                                            {
			                                            return sl.mTraits.mPriority > sr.mTraits.mPriority;
		                                            });
		mNonFixedSystems.insert(whereToInsert, std::move(newInternal));
	}
}

void CE::Registry::CallBeginPlayForEntitiesAwaitingBeginPlay()
{
	auto view = View<Internal::IsAwaitingBeginPlayTag>();

	// We remove all the IsAwaitingBeginPlayTags in a bit.
	// We do this to ensure that if Foo::BeginPlay adds the Bar component,
	// that Bar::BeginPlay will be called immediately after Bar is constructed.
	// So if we don't remove the tags, we might miss some BeginPlay invocations.
	// Buuut, once we remove all of those tags, the view becomes empty.
	// So we make a temporary copy, on the stack to improve performance,
	// before removing all the tags.
	uint32 numOfEntities = static_cast<uint32>(view.size());
	entt::entity* entities = static_cast<entt::entity*>(ENGINE_ALLOCA(sizeof(entt::entity) * numOfEntities));
	{
		uint32 index{};
		for (const entt::entity entity : view)
		{
			entities[index++] = entity;
		}
	}

	RemoveComponents<Internal::IsAwaitingBeginPlayTag>(entities, entities + numOfEntities);

	if (!mWorld.get().HasBegunPlay())
	{
		return;
	}

	World::PushWorld(mWorld);

	for (const BoundEvent& boundEvent : mWorld.get().GetEventManager().GetBoundEvents(sOnBeginPlay))
	{
		entt::sparse_set* storage = Storage(boundEvent.mType.get().GetTypeId());

		if (storage == nullptr)
		{
			continue;
		}

		for (uint32 i = 0; i < numOfEntities; i++)
		{
			entt::entity entity = entities[i];
			if (!storage->contains(entity))
			{
				continue;
			}

			if (boundEvent.mIsStatic)
			{
				boundEvent.mFunc.get().InvokeUncheckedUnpacked(GetWorld(), entity);
			}
			else
			{
				MetaAny component{ boundEvent.mType, storage->value(entity), false };
				boundEvent.mFunc.get().InvokeUncheckedUnpacked(component, GetWorld(), entity);
			}
		}
	}

	World::PopWorld();
}

bool CE::Registry::ShouldWeCallBeginPlayImmediatelyAfterConstruct(entt::entity ownerOfNewlyConstructedComponent) const
{
	return mWorld.get().HasBegunPlay()
		// If our entity is awaiting begin play, we can
		// assume it'll be called later
		&& !HasComponent<Internal::IsAwaitingBeginPlayTag>(ownerOfNewlyConstructedComponent);
}

void CE::Registry::CallEndPlayEventsForEntity(entt::sparse_set& storage, entt::entity entity, const BoundEvent& endPlayEvent)
{
	if (endPlayEvent.mIsStatic)
	{
		endPlayEvent.mFunc.get().InvokeUncheckedUnpacked(GetWorld(), entity);
	}
	else
	{
		MetaAny component{ endPlayEvent.mType, storage.value(entity), false };
		endPlayEvent.mFunc.get().InvokeUncheckedUnpacked(component, GetWorld(), entity);
	}
}

CE::Internal::AnyStorage::AnyStorage(const MetaType& type) :
	BaseType(GetTypeInfo(), entt::deletion_policy::in_place),
	mType(type),
	mTypeInfo(type.GetTypeId(), type.GetTypeInfo(), type.GetName())
{
	if (const std::optional<BoundEvent> boundEvent = TryGetEvent(type, sOnConstruct))
	{
		mOnConstruct = &boundEvent->mFunc.get();
	}

	if (const std::optional<BoundEvent> boundEvent = TryGetEvent(type, sOnBeginPlay))
	{
		mOnBeginPlay = &boundEvent->mFunc.get();
	}

	ASSERT(CanTypeBeUsed(type));
	reserve(64);
}

CE::Internal::AnyStorage::~AnyStorage()
{
	pop_all();
	FastFree(mData);
}

bool CE::Internal::AnyStorage::CanTypeBeUsed(const MetaType& type)
{
	return type.IsDefaultConstructible() && type.IsCopyConstructible() && type.IsMoveConstructible();
}

const void* CE::Internal::AnyStorage::get_at(const std::size_t pos) const
{
	ASSERT(pos < mCapacity);
	return &mData[pos * GetType().GetSize()];
}

void* CE::Internal::AnyStorage::get_at(const std::size_t pos)
{
	ASSERT(pos < mCapacity);
	return &mData[pos * GetType().GetSize()];
}

CE::MetaAny CE::Internal::AnyStorage::element_at(const size_t pos)
{
	return MetaAny{ mType, get_at(pos), false };
}

void CE::Internal::AnyStorage::swap_or_move(const std::size_t from, const std::size_t to)
{
	const MetaType& type = GetType();

	void* src = get_at(from);
	void* dest = get_at(to);

	if (operator[](to) == entt::tombstone) // Move
	{
		// Move construct to dest, which is empty
		type.ConstructAt(static_cast<void*>(dest), MetaAny{ type, src, false });
		type.Destruct(src, false);
	}
	else // Swap
	{
		// Move out of destination
		MetaAny tmp = std::move(type.Construct(MetaAny{ type, dest, false }).GetReturnValue());
		ASSERT(tmp != nullptr);

		// Destination is now 'empty', move assign into it
		MetaAny destRef = { type, dest, false };
		[[maybe_unused]] bool success = !type.Assign(destRef, MetaAny{ type, src, false }).HasError();
		ASSERT(success);

		// Source is now empty, move assign into it
		MetaAny srcRef = { type, src, false };
		success |= !type.Assign(srcRef, std::move(tmp)).HasError();
		ASSERT(success);
	}
}

void CE::Internal::AnyStorage::pop(basic_iterator first, basic_iterator last)
{
	const MetaType& type = GetType();
	for (; first != last; ++first)
	{
		const entt::entity entity = *first;
		MetaAny component = element_at(index(entity));
		type.Destruct(component.GetData(), false);
		in_place_pop(first);
	}
}

void CE::Internal::AnyStorage::pop_all()
{
	const MetaType& type = GetType();

	for (auto first = begin(); !(first.index() < 0); ++first)
	{
		if (*first != entt::tombstone)
		{
			type.Destruct(element_at(index(*first)).GetData(), false);
		}
	}

	BaseType::pop_all();
}

CE::Internal::AnyStorage::BaseType::basic_iterator CE::Internal::AnyStorage::try_emplace(const entt::entity entt, const bool force_back, const void* value)
{
	const MetaType& type = GetType();
	reserve_atleast(size() + 1);

	const auto it = BaseType::try_emplace(entt, force_back);

	void* dest = get_at(it.index());

	if (value != nullptr)
	{
		ASSERT(type.IsCopyConstructible());

		const MetaAny src{ type, const_cast<void*>(value), false };
		[[maybe_unused]] const bool success = !type.ConstructAt(dest, src).HasError();
		ASSERT(success);
	}
	else
	{
		ASSERT(type.IsDefaultConstructible());
		[[maybe_unused]] const bool success = !type.ConstructAt(dest).HasError();
		ASSERT(success);
	}

	return it;
}

void CE::Internal::AnyStorage::reserve(const size_type cap)
{
	BaseType::reserve(cap);

	const MetaType& type = GetType();
	const size_t typeSize = type.GetSize();

	char* const newBuffer = (char*)FastAlloc(cap * typeSize, type.GetAlignment());
	ASSERT(newBuffer != nullptr);
	ASSERT(type.IsMoveConstructible());

	for (auto it = begin(); it != end(); ++it)
	{
		const entt::entity entity = *it;

		if (entity == entt::tombstone)
		{
			continue;
		}

		const size_t placeAtIndex = index(entity);

		void* const src = get_at(placeAtIndex);
		void* const dst = &newBuffer[placeAtIndex * typeSize];
		[[maybe_unused]] const FuncResult result = type.ConstructAt(dst, MetaAny{ type, src, false });
		ASSERT_LOG(!result.HasError(), "{}", result.Error());

		type.Destruct(src, false);
	}

	FastFree(mData);
	mData = newBuffer;
	mCapacity = cap;
}

void CE::Internal::AnyStorage::reserve_atleast(const size_type cap)
{
	if (mCapacity < cap)
	{
		size_t newCapacity = mCapacity;
		do
		{
			newCapacity *= 2;
		} while (newCapacity < cap);

		reserve(newCapacity);
	}
}

CE::MetaType CE::Internal::IsAwaitingBeginPlayTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<IsAwaitingBeginPlayTag>{}, "IsAwaitingBeginPlayTag" };
	metaType.GetProperties().Add(Props::sNoInspectTag).Add(Props::sNoSerializeTag);
	ReflectComponentType<IsAwaitingBeginPlayTag>(metaType);
	return metaType;
}

CE::MetaType CE::Internal::IsAwaitingEndPlayTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<IsAwaitingEndPlayTag>{}, "IsAwaitingEndPlayTag" };
	metaType.GetProperties().Add(Props::sNoInspectTag).Add(Props::sNoSerializeTag);
	ReflectComponentType<IsAwaitingEndPlayTag>(metaType);
	return metaType;
}
