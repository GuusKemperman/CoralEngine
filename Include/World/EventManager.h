#pragma once
#include <execution>

#include "Utilities/Events.h"
#include "Meta/MetaFunc.h"

namespace CE
{
	class World;

	class EventManager
	{
	public:
		EventManager(World& world);
		~EventManager() = default;

		EventManager(EventManager&&) = delete;
		EventManager(const EventManager&) = delete;

		EventManager& operator=(EventManager&&) = delete;
		EventManager& operator=(const EventManager&) = delete;

		template<typename Derived, typename Ret, EventFlags Flags, typename... Params, typename... Args>
		void InvokeEventForAllComponentsOnEntity(const EventType<Derived, Ret(Params...), Flags>& eventType, 
			entt::entity entity, 
			Args&&... args);

		template<bool UseParallelExecution = false, typename Derived, typename Ret, EventFlags Flags, typename... Params, typename... Args>
		void InvokeEventsForAllComponents(const EventType<Derived, Ret(Params...), Flags>& eventType,
			Args&&... args);

		std::span<const BoundEvent> GetBoundEvents(const EventBase& eventBase) const;

	private:
		void InvokeEventForAllComponentsOnEntityImpl(const EventBase& eventBase, 
			entt::entity entity,
			MetaFunc::DynamicArgs args, 
			std::span<TypeForm> argForms);

		void InvokeEventForAllComponentsImpl(const EventBase& eventBase,
			MetaFunc::DynamicArgs args,
			std::span<TypeForm> argForms,
			int8 threadNum = -1);

		std::reference_wrapper<World> mWorld;

		std::unordered_map<std::string_view, std::vector<BoundEvent>> mBoundEvents{};
	};

	template <typename Derived, typename Ret, EventFlags Flags, typename... Params, typename... Args>
	void EventManager::InvokeEventForAllComponentsOnEntity(const EventType<Derived, Ret(Params...), Flags>& eventType,
		entt::entity entity, Args&&... args)
	{
		static_assert(std::is_invocable_v<std::function<void(Params...)>, Args...>);

		std::array<MetaAny, sizeof...(Args) + 3> argsInclusive{
			MetaAny{ MakeTypeInfo<void>(), nullptr }, // This slot can be used for the component if the event is not static
			MetaAny{ mWorld.get() },
			MetaAny{ entity },
			MetaFunc::PackSingle<Args>(std::forward<Args>(args))...
		};

		std::array<TypeForm, sizeof...(Args) + 3> formsInclusive {
			TypeForm::Ref, // This slot can be used for the component if the event is not static
			TypeForm::Ref,
			TypeForm::ConstRef,
			MakeTypeForm<Args>()...
		};

		InvokeEventForAllComponentsOnEntityImpl(eventType, entity, { argsInclusive }, { formsInclusive });
	}

	namespace Internal
	{
		static constexpr size_t sThreadStride = 16;
		static constexpr std::array sThreadOffsets = []
			{
				std::array<int8, sThreadStride> arr{};
				std::iota(arr.begin(), arr.end(), static_cast<int8>(0));
				return arr;
			}();
	}

	template <bool UseParallelExecution, typename Derived, typename Ret, EventFlags Flags, typename ... Params, typename ... Args>
	void EventManager::InvokeEventsForAllComponents(const EventType<Derived, Ret(Params...), Flags>& eventType,
		Args&&... args)
	{
		static_assert(std::is_invocable_v<std::function<void(Params...)>, Args...>);

		auto invoke = [&](int8 offset)
			{
				entt::entity entityBuffer;

				std::array<MetaAny, sizeof...(Args) + 3> argsInclusive{
					MetaAny{ MakeTypeInfo<void>(), nullptr }, // This slot can be used for the component if the event is not static
					MetaAny{ mWorld.get() },
					MetaAny{ MakeTypeInfo<entt::entity>(), &entityBuffer }, // This slot is used for the entt::entity
					MetaFunc::PackSingle<Args>(std::forward<Args>(args))...
				};

				std::array<TypeForm, sizeof...(Args) + 3> formsInclusive{
					TypeForm::Ref, // This slot can be used for the component if the event is not static
					TypeForm::Ref,
					TypeForm::ConstRef,
					MakeTypeForm<Args>()...
				};

				InvokeEventForAllComponentsImpl(eventType,
					{ argsInclusive },
					{ formsInclusive },
					offset);
			};

		if constexpr (UseParallelExecution)
		{
			std::for_each(std::execution::par_unseq, 
				Internal::sThreadOffsets.begin(), Internal::sThreadOffsets.end(), invoke);
		}
		else
		{
			invoke(-1);
		}
	}
}
