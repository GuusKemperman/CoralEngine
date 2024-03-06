#include "Precomp.h"
#include "World/Registry.h"

#include <tinygltf/json.hpp>

#include "Assets/Script.h"
#include "World/World.h"
#include "Assets/Prefabs/ComponentFactory.h"
#include "Assets/Prefabs/Prefab.h"
#include "Assets/Prefabs/PrefabEntityFactory.h"
#include "Components/IsDestroyedTag.h"
#include "Components/PrefabOriginComponent.h"
#include "Components/TransformComponent.h"
#include "World/WorldRenderer.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaAny.h"
#include "Meta/MetaTools.h"
#include "Scripting/ScriptTools.h"
#include "Utilities/Reflect/ReflectComponentType.h"

namespace Engine
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
		const MetaFunc* mOnDestruct{};

		char* mData{};
		size_t mCapacity{};
		entt::type_info mTypeInfo;
	};
}

Engine::Registry::Registry(World& world) :
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

void Engine::Registry::UpdateSystems(float dt)
{
	const std::vector<SingleTick> ticksToCall = GetSortedSystemsToUpdate(dt);
	World& world = GetWorld();

	for (const SingleTick& tick : ticksToCall)
	{
		tick.mSystem.get().Update(world, tick.mDeltaTime);
	}
}

void Engine::Registry::RenderSystems() const
{
	if (!mWorld.get().GetRenderer().GetMainCamera().has_value())
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

entt::entity Engine::Registry::Create()
{
	return mRegistry.create();
}

entt::entity Engine::Registry::Create(entt::entity hint)
{
	return mRegistry.create(hint);
}

entt::entity Engine::Registry::CreateFromPrefab(const Prefab& prefab, entt::entity hint)
{
	const std::vector<PrefabEntityFactory>& factories = prefab.GetFactories();

	if (factories.empty())
	{
		UNLIKELY;
		LOG(LogAssets, Error, "Invalid prefab provided, the prefab {} is empty", prefab.GetName());
		return Create();
	}

	return CreateFromFactory(factories[0], true, hint);
}

entt::entity Engine::Registry::CreateFromFactory(const PrefabEntityFactory& factory, bool createChildren, entt::entity hint)
{
	const entt::entity entity = Create(hint);
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

void Engine::Registry::Destroy(entt::entity entity)
{
	if (!HasComponent<IsDestroyedTag>(entity))
	{
		AddComponent<IsDestroyedTag>(entity);
	}
}

void Engine::Registry::DestroyAlongWithChildren(entt::entity entity)
{
	TransformComponent* transform = TryGet<TransformComponent>(entity);

	if (transform == nullptr)
	{
		Destroy(entity);
		return;
	}

	std::function<void(TransformComponent&)> addToDestroyed = [&](TransformComponent& current)
		{
			for (TransformComponent& child : current.GetChildren())
			{
				addToDestroyed(child);
			}
			Destroy(current.GetOwner());
		};
	addToDestroyed(*transform);
}

void Engine::Registry::RemovedDestroyed()
{
	World::PushWorld(mWorld);
	const auto view = View<const IsDestroyedTag>();

	for (const auto [entity] : view.each())
	{
		mRegistry.destroy(entity);
	}
	World::PopWorld();
}

Engine::MetaAny Engine::Registry::AddComponent(const MetaType& componentClass, const entt::entity toEntity)
{
	if (WasTypeCreatedByScript(componentClass))
	{
		entt::sparse_set* storage = Storage(componentClass.GetTypeId());

		if (storage == nullptr)
		{
			if (!AnyStorage::CanTypeBeUsed(componentClass))
			{
				LOG(LogWorld, Error, "Failed to add component {} to {} - This class was created through scripts, and because of a programmer error it does not have all the functionality a component needs. My bad!",
					componentClass.GetName(),
					static_cast<EntityType>(toEntity));
				return { componentClass, nullptr, false };
			}

			storage = &mRegistry.GuusEngineAddPool<AnyStorage>(componentClass);
			ASSERT(storage != nullptr);
		}
		ASSERT(dynamic_cast<AnyStorage*>(storage) != nullptr);

		storage = Storage(componentClass.GetTypeId());

		AnyStorage* const asAnyStorage = static_cast<AnyStorage*>(storage);
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
			&& GetWorld().HasBegunPlay())
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
			static_cast<EntityType>(toEntity));
		return MetaAny{ componentClass.GetTypeInfo(), nullptr };
	}

	World::PushWorld(mWorld);

	FuncResult result = (*addComponentFunc)(toEntity);

	World::PopWorld();

	if (result.HasError())
	{
		LOG(LogWorld, Error, "Failed to add component {} to {}: Invoking {} failed - {}",
			componentClass.GetName(),
			static_cast<EntityType>(toEntity),
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

void Engine::Registry::RemoveComponent(const TypeId componentClassTypeId, const entt::entity fromEntity)
{
	entt::sparse_set* storage = Storage(componentClassTypeId);
	ASSERT(storage != nullptr);
	storage->erase(fromEntity);
}

void Engine::Registry::RemoveComponentIfEntityHasIt(const TypeId componentClassTypeId, const entt::entity fromEntity)
{
	entt::sparse_set* storage = Storage(componentClassTypeId);

	if (storage != nullptr)
	{
		storage->remove(fromEntity);
	}
}

std::vector<Engine::MetaAny> Engine::Registry::GetComponents(entt::entity entity)
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

Engine::MetaAny Engine::Registry::TryGet(const TypeId componentClassTypeId, entt::entity entity)
{
	const auto storage = Storage(componentClassTypeId);

	if (storage != nullptr
		&& storage->contains(entity))
	{
		return MetaAny{ storage->type().ToGuusTypeInfo<TypeInfo>(), storage->value(entity) };
	}
	return MetaAny{ MakeTypeInfo<void>(), nullptr };
}

Engine::MetaAny Engine::Registry::Get(const TypeId componentClassTypeId, entt::entity entity)
{
	const auto storage = Storage(componentClassTypeId);
	ASSERT(storage != nullptr && storage->contains(entity));
	return MetaAny{ storage->type().ToGuusTypeInfo<TypeInfo>(), storage->value(entity) };
}

bool Engine::Registry::HasComponent(TypeId componentClassTypeId, entt::entity entity) const
{
	const auto storage = Storage(componentClassTypeId);
	return storage != nullptr && storage->contains(entity);
}

void Engine::Registry::Clear()
{
	World::PushWorld(mWorld);
	mRegistry.clear();
	World::PopWorld();
}

std::vector<Engine::Registry::SingleTick> Engine::Registry::GetSortedSystemsToUpdate(const float dt)
{
	std::vector<SingleTick> returnValue{};

	const float currentTime = mWorld.get().GetCurrentTimeScaled();
	const bool hasBegunPlay = GetWorld().HasBegunPlay();
	const bool isPaused = GetWorld().IsPaused();

	const auto canEverTick = [&](const SystemStaticTraits& traits)
		{
			if (hasBegunPlay)
			{
				return !isPaused || traits.mShouldTickWhilstPaused;
			}
			else
			{
				return traits.mShouldTickBeforeBeginPlay;
			}
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

			while (timeOfNextStep <= currentTime)
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

void Engine::Registry::AddSystem(std::unique_ptr<System, InPlaceDeleter<System, true>> system)
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

Engine::AnyStorage::AnyStorage(const MetaType& type) :
	BaseType(GetTypeInfo(), entt::deletion_policy::in_place),
	mType(type),
	mOnConstruct(TryGetEvent(type, sConstructEvent)),
	mOnBeginPlay(TryGetEvent(type, sBeginPlayEvent)),
	mTypeInfo(type.GetTypeId(), type.GetTypeInfo(), type.GetName())
{
	ASSERT(CanTypeBeUsed(type));
	reserve(64);
}

Engine::AnyStorage::~AnyStorage()
{
	pop_all();
	FastFree(mData);
}

bool Engine::AnyStorage::CanTypeBeUsed(const MetaType& type)
{
	return type.IsDefaultConstructible() && type.IsCopyConstructible() && type.IsMoveConstructible();
}

const void* Engine::AnyStorage::get_at(const std::size_t pos) const
{
	ASSERT(pos < mCapacity);
	return &mData[pos * GetType().GetSize()];
}

void* Engine::AnyStorage::get_at(const std::size_t pos)
{
	ASSERT(pos < mCapacity);
	return &mData[pos * GetType().GetSize()];
}

Engine::MetaAny Engine::AnyStorage::element_at(const size_t pos)
{
	return MetaAny{ mType, get_at(pos), false };
}

void Engine::AnyStorage::swap_or_move(const std::size_t from, const std::size_t to)
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
		[[maybe_unused]] bool success = !type.CallFunction(OperatorType::assign, destRef, MetaAny{ type, src, false }).HasError();
		ASSERT(success)

		// Source is now empty, move assign into it
		MetaAny srcRef = { type, src, false };
		success |= !type.CallFunction(OperatorType::assign, srcRef, std::move(tmp)).HasError();
		ASSERT(success);
	}
}

void Engine::AnyStorage::pop(basic_iterator first, basic_iterator last)
{
	ASSERT(World::TryGetWorldAtTopOfStack() != nullptr);
	World& world = *World::TryGetWorldAtTopOfStack();

	const MetaType& type = GetType();
	for (; first != last; ++first)
	{
		const entt::entity entity = *first;
		MetaAny component = element_at(index(entity));
		if (mOnDestruct != nullptr)
		{
			mOnDestruct->InvokeUncheckedUnpacked(component, world, entity);
		}

		type.Destruct(component.GetData(), false);
		in_place_pop(first);
	}
}

void Engine::AnyStorage::pop_all()
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

Engine::AnyStorage::BaseType::basic_iterator Engine::AnyStorage::try_emplace(const entt::entity entt, const bool force_back, const void* value)
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

void Engine::AnyStorage::reserve(const size_type cap)
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

void Engine::AnyStorage::reserve_atleast(const size_type cap)
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
