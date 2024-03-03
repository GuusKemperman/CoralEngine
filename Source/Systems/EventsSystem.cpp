#include "Precomp.h"
#include "Systems/EventsSystem.h"

#include "Utilities/Events.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"

namespace Engine
{
	template<typename EventT, typename Functor>
	static void CallEvent(World& world, const EventT& event, Functor&& functor);
}


void Engine::TickSystem::Update(World& world, float dt)
{
	struct TickFunctor
	{
		FuncResult operator()(const MetaFunc& event, MetaAny& component, entt::entity owner)
		{
			return event.InvokeUncheckedUnpacked(component, mWorldArg, owner, mDtArg);
		}
		MetaAny mWorldArg{ world };
		MetaAny mDtArg{ dt };
	};
	CallEvent(world, sTickEvent, TickFunctor{});
}

void Engine::FixedTickSystem::Update(World& world, float)
{
	struct FixedTickFunctor
	{
		FuncResult operator()(const MetaFunc& event, MetaAny& component, entt::entity owner)
		{
			return event.InvokeUncheckedUnpacked(component, mWorldArg, owner);
		}
		MetaAny mWorldArg{ world };
	};
	CallEvent(world, sFixedTickEvent, FixedTickFunctor{});
}

template<typename EventT, typename Functor>
void Engine::CallEvent(World& world, const EventT& event, Functor&& functor)
{
	Registry& reg = world.GetRegistry();

	// We can't directly iterate over the storages,
	// an OnTick function may spawn a component and
	// construct a new storage, which could invalidate
	// the iterators to the storage itself.

	struct TypeToCallEventFor
	{
		std::reference_wrapper<const MetaType> mType;
		std::reference_wrapper<const MetaFunc> mEvent;
		std::reference_wrapper<entt::sparse_set> mStorage;
	};

	std::vector<TypeToCallEventFor> typesWithEvent{};

	for (auto&& [typeHash, storage] : reg.Storage())
	{
		const MetaType* const type = MetaManager::Get().TryGetType(typeHash);

		if (type == nullptr)
		{
			continue;
		}

		const MetaFunc* const func = TryGetEvent(*type, event);

		if (func != nullptr)
		{
			typesWithEvent.emplace_back(TypeToCallEventFor{ *type, *func, storage });
		}
	}

	for (const auto& [type, func, storage] : typesWithEvent)
	{
		for (const entt::entity entity : storage.get())
		{
			// Tombstone check
			if (!storage.get().contains(entity))
			{
				continue;
			}

			void* ptr = storage.get().value(entity);
			ASSERT(ptr != nullptr);

			MetaAny refToComponent{ type.get(), ptr, false };

			FuncResult result = functor(func, refToComponent, entity);

			if (result.HasError())
			{
				LOG(LogWorld, Error, "An error occured while executing {}::{} of entity {} - {}",
					type.get().GetName(),
					event.mName,
					static_cast<EntityType>(entity),
					result.Error())
			}
		}
	}
}

Engine::MetaType Engine::TickSystem::Reflect()
{
	return MetaType{ MetaType::T<TickSystem>{}, "TickSystem", MetaType::Base<System>{} };
}

Engine::MetaType Engine::FixedTickSystem::Reflect()
{
	return MetaType{ MetaType::T<FixedTickSystem>{}, "FixedTickSystem", MetaType::Base<System>{} };
}
