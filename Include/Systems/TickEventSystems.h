#pragma once
#include "Systems/System.h"
#include "Utilities/Events.h"
#include "World/Registry.h"

#include "entt/entity/runtime_view.hpp"

namespace CE
{
	enum class TickResponsibility
	{
		BeforeBeginPlay,
		WhenPaused,
		WhenRunning
	};

	template<bool IsFixed, TickResponsibility Responsibility>
	class TickEventSystem final :
		public System
	{
	public:
		TickEventSystem();

		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override;

	private:
		friend ReflectAccess;
		static MetaType Reflect();

		std::vector<BoundEvent> mBoundEvents = GetAllBoundEvents(sOnTick);
	};

	template <bool IsFixed, TickResponsibility Responsibility>
	TickEventSystem<IsFixed, Responsibility>::TickEventSystem() :
		mBoundEvents(IsFixed ? GetAllBoundEvents(sOnFixedTick) : GetAllBoundEvents(sOnTick))
	{
		if constexpr (Responsibility == TickResponsibility::BeforeBeginPlay)
		{
			// Remove all the events that are not called before begin play
			mBoundEvents.erase(std::remove_if(mBoundEvents.begin(), mBoundEvents.end(),
				[](const BoundEvent& bound)
				{
					return !bound.mFunc.get().GetProperties().Has(Props::sShouldTickBeforeBeginPlayTag);
				}), mBoundEvents.end());
		}
		else if constexpr (Responsibility == TickResponsibility::WhenPaused)
		{
			// Remove all the events that are not called when paused
			mBoundEvents.erase(std::remove_if(mBoundEvents.begin(), mBoundEvents.end(),
				[](const BoundEvent& bound)
				{
					return !bound.mFunc.get().GetProperties().Has(Props::sShouldTickWhilstPausedTag);
				}), mBoundEvents.end());
		}
	}

	template <bool IsFixed, TickResponsibility Responsibility>
	void TickEventSystem<IsFixed, Responsibility>::Update(World& world, float dt)
	{
		Registry& reg = world.GetRegistry();

		for (const BoundEvent& boundEvent : mBoundEvents)
		{
			entt::sparse_set* const storage = reg.Storage(boundEvent.mType.get().GetTypeId());

			if (storage == nullptr)
			{
				continue;
			}

			entt::runtime_view view{};
			view.iterate(*storage);

			for (const entt::entity entity : view)
			{
				if (boundEvent.mIsStatic)
				{
					boundEvent.mFunc.get().InvokeUncheckedUnpacked(world, entity, dt);
				}
				else
				{
					MetaAny component{ boundEvent.mType.get(), storage->value(entity), false };
					boundEvent.mFunc.get().InvokeUncheckedUnpacked(component, world, entity, dt);
				}
			}
		}
	}

	template <bool IsFixed, TickResponsibility Responsibility>
	SystemStaticTraits TickEventSystem<IsFixed, Responsibility>::GetStaticTraits() const
	{
		SystemStaticTraits traits{};

		if constexpr (IsFixed)
		{
			traits.mFixedTickInterval = sOnFixedTickStepSize;
		}

		traits.mShouldTickBeforeBeginPlay = Responsibility == TickResponsibility::BeforeBeginPlay;
		traits.mShouldTickWhilstPaused = Responsibility == TickResponsibility::WhenPaused;

		return traits;
	}

	template <bool IsFixed, TickResponsibility Responsibility>
	MetaType TickEventSystem<IsFixed, Responsibility>::Reflect()
	{
		return MetaType{ MetaType::T<TickEventSystem>{}, MakeTypeName<TickEventSystem>(), MetaType::Base<System>{} };
	}

	using TickSystemBeforeBeginPlay = TickEventSystem<false, TickResponsibility::BeforeBeginPlay>;
	using TickSystemWhenRunning = TickEventSystem<false, TickResponsibility::WhenRunning>;
	using TickSystemWhenPaused = TickEventSystem<false, TickResponsibility::WhenPaused>;

	using FixedTickSystemBeforeBeginPlay = TickEventSystem<true, TickResponsibility::BeforeBeginPlay>;
	using FixedTickSystemWhenRunning = TickEventSystem<true, TickResponsibility::WhenRunning>;
	using FixedTickSystemWhenPaused = TickEventSystem<true, TickResponsibility::WhenPaused>;

	REFLECT_AT_START_UP(TickSystemBeforeBeginPlay);
	REFLECT_AT_START_UP(TickSystemWhenRunning);
	REFLECT_AT_START_UP(TickSystemWhenPaused);

	REFLECT_AT_START_UP(FixedTickSystemBeforeBeginPlay);
	REFLECT_AT_START_UP(FixedTickSystemWhenRunning);
	REFLECT_AT_START_UP(FixedTickSystemWhenPaused);
}
