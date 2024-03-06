#pragma once
#include "World.h"
#include "Systems/System.h"
#include "Utilities/MemFunctions.h"
#include "Meta/MetaFunc.h"
#include "Meta/MetaManager.h"
#include "Utilities/Events.h"


namespace Engine
{
	class MetaType;
	class MetaAny;

	class Prefab;
	class PrefabEntityFactory;
	class TransformComponent;
	class System;

	// Wrapper around the entt registry.
	class Registry
	{
	public:
		Registry(World& world);

		Registry(Registry&&) = delete;
		Registry(const Registry&) = delete;

		Registry& operator=(Registry&&) = delete;
		Registry& operator=(const Registry&) = delete;

		void UpdateSystems(float dt);

		void RenderSystems() const;

		entt::entity Create();
		
		entt::entity Create(entt::entity hint);

		template<typename It>
		void Create(It first, It last);
		
		entt::entity CreateFromPrefab(const Prefab& prefab, entt::entity hint = entt::null);

		entt::entity CreateFromFactory(const PrefabEntityFactory& factory, bool createChildren, entt::entity hint = entt::null);

		void Destroy(entt::entity entity);
		
		template<typename It>
		void Destroy(It first, It last);

		void DestroyAlongWithChildren(entt::entity entity);

		void RemovedDestroyed();

		template <typename T, typename... Args>
		T& CreateSystem(Args&&... args);

		MetaAny AddComponent(const MetaType& componentClass, entt::entity toEntity);

		template<typename ComponentType, typename ...AdditonalArgs>
		decltype(auto) AddComponent(entt::entity toEntity, AdditonalArgs&& ...additionalArgs);

		template<typename Type, typename It>
		void AddComponents(It first, It last, const Type& value = {});

		void RemoveComponent(TypeId componentClassTypeId, entt::entity fromEntity);

		template<typename ComponentType>
		void RemoveComponent(entt::entity fromEntity);

		void RemoveComponentIfEntityHasIt(TypeId componentClassTypeId, entt::entity fromEntity);

		template<typename ComponentType>
		void RemoveComponentIfEntityHasIt(entt::entity fromEntity);

		std::vector<MetaAny> GetComponents(entt::entity entity);

		World& GetWorld() { return mWorld; }

		const World& GetWorld() const { return mWorld; }

		template<typename Type, typename... Other, typename... Exclude>
		auto View(entt::exclude_t<Exclude...> exclude = entt::exclude_t{}) const { return mRegistry.view<Type, Other...>(exclude); }

		template<typename Type, typename... Other, typename... Exclude>
		auto View(entt::exclude_t<Exclude...> exclude = entt::exclude_t{}) { return mRegistry.view<Type, Other...>(exclude); }

		template<typename Type>
		entt::registry::storage_for_type<Type>& Storage() { return mRegistry.storage<Type>(); }

		template<typename Type>
		const entt::registry::storage_for_type<Type>* Storage() const { return mRegistry.storage<Type>(); }

		template<typename Type>
		Type* TryGet(entt::entity entity) { return mRegistry.try_get<Type>(entity); }

		template<typename Type>
		const Type* TryGet(entt::entity entity) const { return mRegistry.try_get<Type>(entity); }

		MetaAny TryGet(TypeId componentClassTypeId, entt::entity entity);

		template<typename Type>
		Type& Get(entt::entity entity) { ASSERT(TryGet<Type>(entity)); return mRegistry.get<Type>(entity); }

		template<typename Type>
		const Type& Get(entt::entity entity) const { ASSERT(TryGet<Type>(entity)); return mRegistry.get<Type>(entity); }

		template<typename Type>
		Type& GetOrAdd(entt::entity entity);

		MetaAny Get(TypeId componentClassTypeId, entt::entity entity);

		template<typename Type>
		bool HasComponent(entt::entity entity) const;

		bool HasComponent(TypeId componentClassTypeId, entt::entity entity) const;

		bool Valid(entt::entity entity) const { return mRegistry.valid(entity); }

		auto Storage() { return mRegistry.storage(); }

		auto Storage() const { return mRegistry.storage(); }

		entt::sparse_set* Storage(const TypeId componentClassTypeId) { return mRegistry.storage(componentClassTypeId); }

		const entt::sparse_set* Storage(const TypeId componentClassTypeId) const { return mRegistry.storage(componentClassTypeId); }

		void Clear();

	private:
		struct SingleTick
		{
			SingleTick(System& system, float deltaTime = 0.0f) : mSystem(system), mDeltaTime(deltaTime) {};
			std::reference_wrapper<System> mSystem;
			float mDeltaTime{};
		};
		std::vector<SingleTick> GetSortedSystemsToUpdate(float deltaTime);
		
		void AddSystem(std::unique_ptr<System, InPlaceDeleter<System, true>> system);

		// mWorld needs to be updated in World::World(World&&), so we give access to World to do so.
		friend class World;
		std::reference_wrapper<World> mWorld; 
		
		entt::registry mRegistry;

		struct InternalSystem
		{
			InternalSystem(std::unique_ptr<System, InPlaceDeleter<System, true>> system, SystemStaticTraits traits) :
				mSystem(std::move(system)),
				mTraits(traits) {}

			// Systems are created using the runtime reflection system,
			// which uses placement new for the constructing of objects.
			// Hence, the custom deleter
			std::unique_ptr<System, InPlaceDeleter<System, true>> mSystem{};
			SystemStaticTraits mTraits{};
		};
		
		struct FixedTickSystem :
			public InternalSystem
		{
			FixedTickSystem(std::unique_ptr<System, InPlaceDeleter<System, true>> system, SystemStaticTraits traits, float timeOfNextStep) :
				InternalSystem(std::move(system), traits),
				mTimeOfNextStep(timeOfNextStep) {}
			float mTimeOfNextStep{};
		};
		std::vector<FixedTickSystem> mFixedTickSystems{};
		std::vector<InternalSystem> mNonFixedSystems{};
	};

	template<typename ComponentType, typename ...AdditonalArgs>
	decltype(auto) Registry::AddComponent(const entt::entity toEntity, AdditonalArgs && ...additionalArgs)
	{
		struct ComponentEvents
		{
			const MetaType* mType{};
			const MetaFunc* mOnConstruct{};
			const MetaFunc* mOnBeginPlay{};
		};

		static const ComponentEvents events =
			[]
			{
				if constexpr (sIsReflectable<ComponentType>)
				{
					ComponentEvents tmpEvents{};
					tmpEvents.mType = &MetaManager::Get().GetType<ComponentType>();
					tmpEvents.mOnConstruct = TryGetEvent(*tmpEvents.mType, sConstructEvent);
					tmpEvents.mOnBeginPlay = TryGetEvent(*tmpEvents.mType, sBeginPlayEvent);
					return tmpEvents;
				}
				else
				{
					return ComponentEvents{};
				}
			}();

		static constexpr bool isEmpty = entt::component_traits<ComponentType>::page_size == 0;

		if constexpr (isEmpty)
		{
			mRegistry.emplace<ComponentType>(toEntity, std::forward<AdditonalArgs>(additionalArgs)...);

			if constexpr (sIsReflectable<ComponentType>)
			{
				if (events.mOnConstruct != nullptr)
				{
					events.mOnConstruct->InvokeUncheckedUnpacked(GetWorld(), toEntity);
				}

				if (GetWorld().HasBegunPlay()
					&& events.mOnBeginPlay != nullptr)
				{
					events.mOnBeginPlay->InvokeUncheckedUnpacked(GetWorld(), toEntity);
				}
			}
		}
		else
		{
			ComponentType& component = mRegistry.emplace<ComponentType>(toEntity, std::forward<AdditonalArgs>(additionalArgs)...);;

			if constexpr (sIsReflectable<ComponentType>)
			{
				if (events.mOnConstruct != nullptr)
				{
					events.mOnConstruct->InvokeUncheckedUnpacked(component, GetWorld(), toEntity);
				}

				if (GetWorld().HasBegunPlay()
					&& events.mOnBeginPlay != nullptr)
				{
					events.mOnBeginPlay->InvokeUncheckedUnpacked(component, GetWorld(), toEntity);
				}
			}

			return component;
		}
	}

	template<typename Type, typename It>
	void Registry::AddComponents(It first, It last, const Type& value)
	{
		mRegistry.insert(std::move(first), std::move(last), value);
	}

	template<typename Type>
	Type& Registry::GetOrAdd(entt::entity entity)
	{
		Type* existing = TryGet<Type>(entity);
		if (existing == nullptr)
		{
			return AddComponent<Type>(entity);
		}
		return *existing;
	}

	template <typename Type>
	bool Registry::HasComponent(entt::entity entity) const
	{
		auto* storage = Storage<Type>();
		return storage != nullptr && storage->contains(entity);
	}

	template<typename ComponentType>
	void Registry::RemoveComponent(entt::entity fromEntity)
	{
		mRegistry.erase<ComponentType>(fromEntity);
	}

	template<typename ComponentType>
	void Registry::RemoveComponentIfEntityHasIt(entt::entity fromEntity)
	{
		mRegistry.remove<ComponentType>(fromEntity);
	}

	template<typename It>
	void Registry::Create(It first, It last)
	{
		mRegistry.create(first, last);
	}

	template<typename It>
	void Registry::Destroy(It first, It last)
	{
		for (auto it = first; it != last; ++it)
		{
			Destroy(*it);
		}
	}

	template <typename T, typename... Args>
	T& Registry::CreateSystem(Args&&... args)
	{
		// We could do std::make_unique in this case, we also create
		// systems from a metatype. Metatypes construct using placement
		// new and can thus not construct a normal, default deleter unique
		// ptr, which is why we cannot do that here either.
		void* buffer = FastAlloc(sizeof(T), alignof(T));
		ASSERT(buffer != nullptr);

		T* obj = new (buffer) T(std::forward<Args>(args)...);
		std::unique_ptr<System, InPlaceDeleter<System, true>> newSystem{ obj };
		AddSystem(std::move(newSystem));

		return *obj;
	}
}
