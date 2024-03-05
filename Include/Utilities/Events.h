#pragma once
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

namespace Engine
{
	class World;

	class EventBase
	{
	public:
		std::string_view mName{};
	};

	template<typename... Args>
	class Event :
		public EventBase
	{
	};

	//********************************//
	//				API				  //
	//********************************//

	template<typename T>
	static constexpr bool sIsEventStatic = entt::component_traits<T>::page_size == 0;

	namespace Props
	{
		static constexpr std::string_view sIsEventStaticTag = "sIsEventStaticTag";
	}

	/**
	 * \brief Called every frame.
	 * \World& The world this component is in. 
	 * \entt::entity The owner of this component. 
	 * \float The deltatime. 
	 */
	static constexpr Event<World&, entt::entity, float> sTickEvent{ "OnTick" };

	/**
	 * \brief Called every sFixedTickEventStepSize seconds.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	static constexpr Event<World&, entt::entity> sFixedTickEvent{ "OnFixedTick" };

	/**
	 * \brief The number of seconds between fixed ticks.
	 */
	static constexpr float sFixedTickEventStepSize = 0.2f;

	static constexpr Event<World&, entt::entity> sOnConstructEvent{ "OnConstruct" };

	static constexpr Event<World&, entt::entity> sOnBeginPlayEvent{ "OnConstruct" };

	/**
	 * \brief Binds an event to a type.
	 * \tparam Class The class that is being reflected.
	 * \tparam Args The arguments of the events. Note that each event also requires the first argument to be Class& (see Class template argument).
	 * \param type A type that is currently being reflected.
	 * \param event The event you would like to bind the function to, such as sTickEvent.
	 * \param func The function you want to bind to the event. Everytime the event is dispatched, the function is called.
	 *
	 *	Events can be bound from C++ in the Reflect function.
	 *
	 *	class EventTestingComponent
	 *	{
	 *	public:
	 *		void OnTick(World& world, entt::entity owner, float dt);
	 *	};
	 *  
	 *	Engine::MetaType Engine::EventTestingComponent::Reflect()
	 *	{
	 *		MetaType type = MetaType{ MetaType::T<EventTestingComponent>{}, "EventTestingComponent" };
	 *
	 *		BindEvent(type, sTickEvent, &EventTestingComponent::OnTick);
	 *
	 *		ReflectComponentType<EventTestingComponent>(type);
	 *		return type;
	 *	}
	 */
	template<typename Class, typename Func, typename... Args>
	void BindEvent(MetaType& type, const Event<Args...>& event, Func&& func);

	/**
	 * \brief An overload that prevents having to specify Class for mutable member functions
	 */
	template<typename FuncRet, typename FuncObj, typename... FuncParams, typename... EventParams>
	void BindEvent(MetaType& type, const Event<EventParams...>& event, FuncRet(FuncObj::* func)(FuncParams...));

	/**
	 * \brief An overload that prevents having to specify Class for const member functions
	 */
	template<typename FuncRet, typename FuncObj, typename... FuncParams, typename... EventParams>
	void BindEvent(MetaType& type, const Event<EventParams...>& event, FuncRet(FuncObj::* func)(FuncParams...) const);

	template<typename FuncRet, typename... FuncParams, typename... EventParams>
	void BindEvent(MetaType& type, const Event<EventParams...>& event, FuncRet(*func)(FuncParams...));

	/**
	 * \brief Returns the event bound during BindEvent, if any.
	 */
	template<typename... Args>
	const MetaFunc* TryGetEvent(const MetaType& fromType, const Event<Args...>& event);

	//********************************//
	//			Implementation		  //
	//********************************//

	namespace Internal
	{
		static constexpr std::string_view sIsEventProp = "IsEvent";
	}

	template<typename Class, typename Func, typename... Args>
	void BindEvent(MetaType& type, const Event<Args...>& event, Func&& func)
	{
		MetaFunc* eventFunc{};

		if constexpr (!sIsEventStatic<Class>)
		{
			ASSERT(type.GetTypeId() == MakeTypeId<Class>());

			eventFunc = &type.AddFunc(std::function<void(Class&, Args...)>{ std::forward<Func>(func) }, event.mName);
		}
		else
		{
			eventFunc = &type.AddFunc(std::function<void(Args...)>{ std::forward<Func>(func) }, event.mName);
			eventFunc->GetProperties().Add(Props::sIsEventStaticTag);
		}
		eventFunc->GetProperties().Add(Internal::sIsEventProp);
	}

	template <typename FuncRet, typename FuncObj, typename ... FuncParams, typename ... EventParams>
	void BindEvent(MetaType& type, const Event<EventParams...>& event, FuncRet(FuncObj::* func)(FuncParams...))
	{
		BindEvent<FuncObj>(type, event, func);
	}

	template <typename FuncRet, typename FuncObj, typename ... FuncParams, typename ... EventParams>
	void BindEvent(MetaType& type, const Event<EventParams...>& event, FuncRet(FuncObj::* func)(FuncParams...) const)
	{
		BindEvent<FuncObj>(type, event, func);
	}

	template <typename FuncRet, typename ... FuncParams, typename ... EventParams>
	void BindEvent(MetaType& type, const Event<EventParams...>& event, FuncRet(* func)(FuncParams...))
	{
		BindEvent<std::monostate>(type, event, func);
	}

	template <typename ... Args>
	const MetaFunc* TryGetEvent(const MetaType& fromType, const Event<Args...>& event)
	{
		const MetaFunc* func = fromType.TryGetFunc(event.mName);

		if (func != nullptr
			&& func->GetProperties().Has(Internal::sIsEventProp))
		{
			return func;
		}
		return nullptr;
	}
}
