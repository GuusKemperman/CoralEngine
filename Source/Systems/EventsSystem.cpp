#include "Precomp.h"
#include "Systems/EventsSystem.h"

#include "Utilities/Events.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"

namespace CE
{
	template <typename Functor>
	static void CallEvent(World& world, const std::vector<BoundEvent>& bound, Functor&& functor);
}

void CE::TickSystem::Update(World& world, float dt)
{
	struct TickFunctor
	{
		TickFunctor(MetaAny&& world, MetaAny&& dtArg)
			:
			mWorldArg(std::move(world)),
			mDtArg(std::move(dtArg))
		{
		}

		FuncResult operator()(const MetaFunc& event, std::optional<MetaAny>&& component, entt::entity owner)
		{
			if (component.has_value())
			{
				return event.InvokeUncheckedUnpacked(*component, mWorldArg, owner, mDtArg);
			}
			return event.InvokeUncheckedUnpacked(mWorldArg, owner, mDtArg);
		}

		MetaAny mWorldArg;
		MetaAny mDtArg;
	};

	CallEvent(world, mBoundEvents, TickFunctor{MetaAny{world}, MetaAny{dt}});
}

void CE::FixedTickSystem::Update(World& world, float)
{
	struct FixedTickFunctor
	{
		FixedTickFunctor(MetaAny&& worldArg)
			:
			mWorldArg(std::move(worldArg))
		{
		}

		FuncResult operator()(const MetaFunc& event, std::optional<MetaAny>&& component, entt::entity owner)
		{
			if (component.has_value())
			{
				return event.InvokeUncheckedUnpacked(*component, mWorldArg, owner);
			}
			return event.InvokeUncheckedUnpacked(mWorldArg, owner);
		}

		MetaAny mWorldArg;
	};
	CallEvent(world, mBoundEvents, FixedTickFunctor{MetaAny{world}});
}

template <typename Functor>
void CE::CallEvent(World& world, const std::vector<BoundEvent>& boundEvents, Functor&& functor)
{
	Registry& reg = world.GetRegistry();

	for (const BoundEvent& boundEvent : boundEvents)
	{
		entt::sparse_set* const storage = reg.Storage(boundEvent.mType.get().GetTypeId());

		if (storage == nullptr)
		{
			continue;
		}

		for (const entt::entity entity : *storage)
		{
			// Tombstone check
			if (!storage->contains(entity))
			{
				continue;
			}
			if (boundEvent.mIsStatic)
			{
				functor(boundEvent.mFunc, std::nullopt, entity);
			}
			else
			{
				functor(boundEvent.mFunc, MetaAny{ boundEvent.mType.get(), storage->value(entity), false }, entity);
			}
		}
	}
}

CE::MetaType CE::TickSystem::Reflect()
{
	return MetaType{MetaType::T<TickSystem>{}, "TickSystem", MetaType::Base<System>{}};
}

CE::MetaType CE::FixedTickSystem::Reflect()
{
	return MetaType{MetaType::T<FixedTickSystem>{}, "FixedTickSystem", MetaType::Base<System>{}};
}
