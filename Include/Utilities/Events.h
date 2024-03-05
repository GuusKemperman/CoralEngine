#pragma once
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

namespace Engine
{
	class World;

	class EventBase
	{
	public:
		constexpr EventBase(std::string_view name, bool isPure);

		std::string_view mName{};
		bool mIsPure{};
	};

	template<typename T, bool IsPure = false>
	class Event
	{
		static_assert(AlwaysFalse<T>, "Not a signature");
	};

	template<typename Ret, typename... Args, bool IsPure>
	class Event<Ret(Args...), IsPure> :
		public EventBase
	{
	public:
		constexpr Event(std::string_view name);
	};

	constexpr EventBase::EventBase(std::string_view name, bool isPure) :
		mName(name),
		mIsPure(isPure)
	{
	}

	template <typename Ret, typename ... Args, bool IsPure>
	constexpr Event<Ret(Args...), IsPure>::Event(std::string_view name) :
		EventBase(name, IsPure)
	{
	}

	//********************************//
	//				API				  //
	//********************************//

	template<typename T>
	static constexpr bool sIsEventStatic = entt::component_traits<T>::page_size == 0;

	namespace Props
	{
		static constexpr std::string_view sIsEventStaticTag = "sIsEventStaticTag";
	}

	static constexpr Event<float(const World&, entt::entity), true> sAIEvaluateEvent{ "OnAIEvaluate" };

	static constexpr Event<void(World&, entt::entity, float)> sAITickEvent{ "OnAITick" };

	/**
	 * \brief Called every frame.
	 * \World& The world this component is in. 
	 * \entt::entity The owner of this component. 
	 * \float The deltatime. 
	 */
	static constexpr Event<void(World&, entt::entity, float)> sTickEvent{ "OnTick" };

	/**
	 * \brief Called every sFixedTickEventStepSize seconds.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	static constexpr Event<void(World&, entt::entity)> sFixedTickEvent{ "OnFixedTick" };

	/**
	 * \brief The number of seconds between fixed ticks.
	 */
	static constexpr float sFixedTickEventStepSize = 0.2f;

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
	template<typename Class, typename Func, typename Ret, typename... Args, bool IsPure>
	void BindEvent(MetaType& type, const Event<Ret(Args...), IsPure>& event, Func&& func);

	/**
	 * \brief An overload that prevents having to specify Class for mutable member functions
	 */
	template<typename FuncRet, typename FuncObj, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet(FuncObj::* func)(FuncParams...));

	/**
	 * \brief An overload that prevents having to specify Class for const member functions
	 */
	template<typename FuncRet, typename FuncObj, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet(FuncObj::* func)(FuncParams...) const);

	template<typename FuncRet, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet(*func)(FuncParams...));

	/**
	 * \brief Returns the event bound during BindEvent, if any.
	 */
	template<typename T>
	const MetaFunc* TryGetEvent(const MetaType& fromType, const Event<T>& event);

	//********************************//
	//			Implementation		  //
	//********************************//



	namespace Internal
	{
		static constexpr std::string_view sIsEventProp = "IsEvent";
	}

	template<typename Class, typename Func, typename Ret, typename... Args, bool IsPure>
	void BindEvent(MetaType& type, const Event<Ret(Args...), IsPure>& event, Func&& func)
	{
		static_assert(std::is_const_v<Class> == IsPure, "Cannot be bound to function, make the function const (or remove const)");

		MetaFunc* eventFunc{};

		if constexpr (!sIsEventStatic<Class>)
		{
			ASSERT(type.GetTypeId() == MakeTypeId<Class>());

			eventFunc = &type.AddFunc(std::function<Ret(Class&, Args...)>{ std::forward<Func>(func) }, event.mName);
		}
		else
		{
			eventFunc = &type.AddFunc(std::function<Ret(Args...)>{ std::forward<Func>(func) }, event.mName);
			eventFunc->GetProperties().Add(Props::sIsEventStaticTag);
		}
		eventFunc->GetProperties().Add(Internal::sIsEventProp).Set(Props::sIsScriptPure, IsPure);
	}


	template<typename FuncRet, typename FuncObj, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet(FuncObj::* func)(FuncParams...))
	{
		BindEvent<FuncObj>(type, event, func);
	}

	template<typename FuncRet, typename FuncObj, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet(FuncObj::* func)(FuncParams...) const)
	{
		BindEvent<FuncObj>(type, event, func);
	}

	template<typename FuncRet, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet(*func)(FuncParams...))
	{
		BindEvent<std::monostate>(type, event, func);
	}

	template <typename T>
	const MetaFunc* TryGetEvent(const MetaType& fromType, const Event<T>& event)
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
