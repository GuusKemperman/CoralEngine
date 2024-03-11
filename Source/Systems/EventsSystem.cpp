#include "Precomp.h"
#include "Systems/EventsSystem.h"

#include "Utilities/Events.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"

namespace Engine
{
	template <typename EventT, typename Functor>
	static void CallEvent(World& world, const EventT& event, Functor&& functor);
}

void Engine::TickSystem::Update(World& world, float dt)
{
	struct TickFunctor
	{
		TickFunctor(MetaAny&& world, MetaAny&& dtArg)
			:
			mWorldArg(std::move(world)),
			mDtArg(std::move(dtArg))
		{}

		FuncResult operator()(const MetaFunc& event, std::optional<MetaAny>&& component, entt::entity owner)
		{
			if (component.has_value())
			{
				return event.InvokeUncheckedUnpacked(*component, mWorldArg, owner, mDtArg);
			}
			return event.InvokeUncheckedUnpacked(mWorldArg, owner, mDtArg);
		}

		MetaAny mWorldArg{world};
		MetaAny mDtArg{dt};
		MetaAny mWorldArg;
		MetaAny mDtArg;
	};
	
	CallEvent(world, sTickEvent, TickFunctor{ MetaAny{world}, MetaAny{dt} });
}

void Engine::FixedTickSystem::Update(World& world, float)
{
	struct FixedTickFunctor
	{
		FixedTickFunctor(MetaAny&& worldArg)
			:
			mWorldArg(std::move(worldArg))
		{}

		FuncResult operator()(const MetaFunc& event, std::optional<MetaAny>&& component, entt::entity owner)
		{
			if (component.has_value())
			{
				return event.InvokeUncheckedUnpacked(*component, mWorldArg, owner);
			}
			return event.InvokeUncheckedUnpacked(mWorldArg, owner);
		}
		MetaAny mWorldArg{world};
		MetaAny mWorldArg;
	};
	CallEvent(world, sFixedTickEvent, FixedTickFunctor{ MetaAny{world} });
}

template <typename EventT, typename Functor>
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
			typesWithEvent.emplace_back(TypeToCallEventFor{*type, *func, storage});
		}
	}

	for (const auto& [type, func, storage] : typesWithEvent)
	{
		const bool isStatic = func.get().GetProperties().Has(Props::sIsEventStaticTag);

		for (const entt::entity entity : storage.get())
		{
			// Tombstone check
			if (!storage.get().contains(entity))
			{
				continue;
			}
			if (isStatic)
			{
				functor(func, std::nullopt, entity);
			}
			else
			{
				functor(func, MetaAny{type.get(), storage.get().value(entity), false}, entity);
			}
		}
	}
}

Engine::MetaType Engine::TickSystem::Reflect()
{
	return MetaType{MetaType::T<TickSystem>{}, "TickSystem", MetaType::Base<System>{}};
}

Engine::MetaType Engine::FixedTickSystem::Reflect()
{
	return MetaType{MetaType::T<FixedTickSystem>{}, "FixedTickSystem", MetaType::Base<System>{}};
}
