#pragma once
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
		void InvokeEventForAllComponentsOnEntity(const EventType<Derived, Ret(Params...), Flags>& eventType, entt::entity entity, Args&&... args);

		std::span<const BoundEvent> GetBoundEvents(const EventBase& eventBase) const;

	private:
		void InvokeEventForAllComponentsOnEntityImpl(const EventBase& eventBase, 
			entt::entity entity,
			MetaFunc::DynamicArgs argsProvided, 
			std::span<TypeForm> argFormsProvided);

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
}
