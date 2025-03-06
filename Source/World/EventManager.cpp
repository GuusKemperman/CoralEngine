#include "Precomp.h"
#include "World/EventManager.h"

#include "entt/entity/runtime_view.hpp"

#include "World/Registry.h"
#include "World/World.h"

CE::EventManager::EventManager(World& world) :
	mWorld(world)
{
	const std::span<std::reference_wrapper<const EventBase>> allEvents = GetAllEvents();

	mBoundEvents.reserve(allEvents.size());

	for (const EventBase& eventBase : allEvents)
	{
		mBoundEvents.emplace(eventBase.mName, GetAllBoundEventsSlow(eventBase));
	}
}

std::span<const CE::BoundEvent> CE::EventManager::GetBoundEvents(const EventBase& eventBase) const
{
	const auto boundEvents = mBoundEvents.find(eventBase.mName);
	ASSERT_LOG(boundEvents != mBoundEvents.end(), "Could not find bound events for {}", eventBase.mName);
	return boundEvents->second;
}

void CE::EventManager::InvokeEventForAllComponentsOnEntityImpl(const EventBase& eventBase, entt::entity entity,
                                                               MetaFunc::DynamicArgs args, std::span<TypeForm> argForms)
{
	for (const BoundEvent& boundEvent : GetBoundEvents(eventBase))
	{
		entt::sparse_set* const storage = mWorld.get().GetRegistry().Storage(boundEvent.mType.get().GetTypeId());

		if (storage == nullptr
			|| !storage->contains(entity))
		{
			continue;
		}

		const MetaFunc& func = boundEvent.mFunc.get();

		if (boundEvent.mIsStatic)
		{
			func.InvokeUnchecked({ args.data() + 1, args.size() - 1 }, { argForms.data() + 1, argForms.size() - 1 });
		}
		else
		{
			args[0].AssignFromAnyOfDifferentType({ boundEvent.mType, storage->value(entity), false });
			func.InvokeUnchecked(args, argForms);
		}
	}
}

void CE::EventManager::InvokeEventForAllComponentsImpl(const EventBase& eventBase,
	MetaFunc::DynamicArgs args,
	std::span<TypeForm> argForms,
	int8 threadNum)
{
	size_t executedNum{};

	for (const BoundEvent& boundEvent : GetBoundEvents(eventBase))
	{
		entt::sparse_set* const storage = mWorld.get().GetRegistry().Storage(boundEvent.mType.get().GetTypeId());

		if (storage == nullptr)
		{
			continue;
		}

		const MetaFunc& func = boundEvent.mFunc.get();

		entt::runtime_view view{};
		view.iterate(*storage);

		for (const entt::entity entity : view)
		{
			if (threadNum != -1
				&& executedNum++ % Internal::sThreadStride != threadNum)
			{
				continue;
			}

			*static_cast<entt::entity*>(args[2].GetData()) = entity;

			if (boundEvent.mIsStatic)
			{
				func.InvokeUnchecked({ args.data() + 1, args.size() - 1 }, { argForms.data() + 1, argForms.size() - 1 });
			}
			else
			{
				args[0].AssignFromAnyOfDifferentType({ boundEvent.mType, storage->value(entity), false });
				func.InvokeUnchecked(args, argForms);
			}
		}
	}
}
