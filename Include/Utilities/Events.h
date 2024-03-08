#pragma once
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

namespace Engine
{
	class World;

	class EventBase
	{
	public:
		constexpr EventBase(std::string_view name, bool isPure, bool isAlwaysStatic);

		std::string_view mName{};
		bool mIsPure{};
		bool mIsAlwaysStatic{};
	};

	template<typename T, bool IsPure = false, bool IsAlwaysStatic = false>
	class Event
	{
		static_assert(AlwaysFalse<T>, "Not a signature");
	};

	template<typename Ret, typename... Args, bool IsPure, bool IsAlwaysStatic>
	class Event<Ret(Args...), IsPure, IsAlwaysStatic> :
		public EventBase
	{
	public:
		constexpr Event(std::string_view name);
	};

	constexpr EventBase::EventBase(std::string_view name, bool isPure, bool isAlwaysStatic) :
		mName(name),
		mIsPure(isPure),
		mIsAlwaysStatic(isAlwaysStatic)
	{
	}

	template <typename Ret, typename ... Args, bool IsPure, bool IsAlwaysStatic>
	constexpr Event<Ret(Args...), IsPure, IsAlwaysStatic>::Event(std::string_view name) :
		EventBase(name, IsPure, IsAlwaysStatic)
	{
	}

	//********************************//
	//				API				  //
	//********************************//

	namespace Props
	{
		static constexpr std::string_view sIsEventStaticTag = "sIsEventStaticTag";
	}

	static constexpr Event<float(const World&, entt::entity), true> sAIEvaluateEvent{ "OnAIEvaluate" };

	static constexpr Event<void(World&, entt::entity, float)> sAITickEvent{ "OnAITick" };

	/**
	 * \brief 
	 * 	World& The world the ability controller component is in. 
	 * \entt::entity The owner of the ability controller component.  
	 */
	static constexpr Event<void(World&, entt::entity), false, true> sAbilityActivateEvent{ "OnAbilityActivate" };

	/**
	 * \brief Called immediately when the component is constructed.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	static constexpr Event<void(World&, entt::entity)> sConstructEvent{ "OnConstruct" };

	/**
	 * \brief Called when the world begins play, or if the component is created after the world has begun play. Called after OnConstruct.
	 *
	 * Note that the order of BeginPlay is undefined; if you add a prefab to a world that has already begun play, you have no guarantee
	 * during OnBeginPlay that each component of the prefab has been added to the entity.
	 *
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	static constexpr Event<void(World&, entt::entity)> sBeginPlayEvent{ "OnBeginPlay" };

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
	 * \brief Called just before the C++ destructor. 
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	static constexpr Event<void(World&, entt::entity)> sDestructEvent{ "OnDestruct" };

	/**
	 * \brief Called the first frame two entities are colliding
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 * \entt::entity The entity this entity collided with.
	 * \float The depth of the collision, how far it penetrated
	 * \glm::vec2 The collision normal
	 * \glm::vec2 The point of contact
	 */
	static constexpr Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)> sCollisionEntryEvent{ "OnCollisionEntry" };

	/**
	 * \brief Called every frame in which two entities are colliding. ALWAYS called after at one OnCollisionEntry
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 * \entt::entity The entity this entity collided with.
	 * \float The depth of the collision, how far it penetrated
	 * \glm::vec2 The collision normal
	 * \glm::vec2 The point of contact
	 */
	static constexpr Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)> sCollisionStayEvent{ "OnCollisionStay" };

	/**
	 * \brief Called the first frame that two entities who were colliding in the previous frame, but no longer are.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 * \entt::entity The entity this entity collided with.
	 * \float The depth of the collision, how far it penetrated
	 * \glm::vec2 The collision normal
	 * \glm::vec2 The point of contact
	 */
	static constexpr Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)> sCollisionExitEvent{ "OnCollisionExit" };

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
	template<typename Class, typename Func, typename Ret, typename... Args, bool IsPure, bool IsAlwaysStatic>
	void BindEvent(MetaType& type, const Event<Ret(Args...), IsPure, IsAlwaysStatic>& event, Func&& func);

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
	 *
	 *  Example: TryGetEvent(componentType, sFixedTickEvent);
	 */
	template<typename EventT>
	const MetaFunc* TryGetEvent(const MetaType& fromType, const EventT& event);

	//********************************//
	//			Implementation		  //
	//********************************//

	namespace Internal
	{
		static constexpr std::string_view sIsEventProp = "IsEvent";
	}

	template<typename Class, typename Func, typename Ret, typename... Args, bool IsPure, bool IsAlwaysStatic>
	void BindEvent(MetaType& type, const Event<Ret(Args...), IsPure, IsAlwaysStatic>& event, Func&& func)
	{
		static_assert(std::is_const_v<Class> == IsPure, "Cannot be bound to function, make the function const (or remove const)");

		MetaFunc* eventFunc{};

		if constexpr (entt::component_traits<Class>::page_size != 0
			&& !IsAlwaysStatic)
		{
			ASSERT(type.GetTypeId() == MakeTypeId<Class>());
			static_assert(std::is_invocable_v<decltype(func), Class&, Args...>, "The parameters of the event do not match the parameters of the function");

			eventFunc = &type.AddFunc(std::function<Ret(Class&, Args...)>{ std::forward<Func>(func) }, event.mName);
		}
		else
		{
			static_assert(std::is_invocable_v<decltype(func), Args...>, "The parameters of the event do not match the parameters of the function");

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

	template <typename EventT>
	const MetaFunc* TryGetEvent(const MetaType& fromType, const EventT& event)
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
