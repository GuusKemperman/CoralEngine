#pragma once
#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

namespace CE
{
	class Script;
	class ScriptFunc;
	class World;

	enum class EventFlags
	{
		Default = 0,
		NotScriptable = 1 << 1
	};

	template<typename Derived, typename T = void(), EventFlags = EventFlags::Default>
	class EventType
	{
		static_assert(AlwaysFalse<T>, "Not a signature");
	};

	class EventBase
	{
	public:
		EventBase(std::string_view name, 
			EventFlags flags,
			TypeTraits eventReturnType,
			std::vector<TypeTraits>&& eventParams,
			std::vector<std::string_view>&& pinNames);

		EventBase(const EventBase&) = delete;
		EventBase(EventBase&&) = delete;

		EventBase& operator=(const EventBase&) = delete;
		EventBase& operator=(EventBase&&) = delete;

		MetaFunc& Declare(TypeId selfTypeId, MetaType& toType) const;
		void Define(MetaFunc& metaFunc, const ScriptFunc& scriptFunc, const AssetHandle<Script>& script) const;

#ifdef EDITOR
		/**
		 * \brief Can be used to make calls to ImGui so the user can add/remove event specific properties.
		 */
		virtual void OnDetailsInspect([[maybe_unused]] ScriptFunc& scriptFunc) const {}
#endif // EDITOR

		std::string_view mName{};
		EventFlags mFlags{};

		// Equal to MakeTypeTraits<void>() if this function
		// has no return value.
		TypeTraits mEventReturnType{};

		// EventParams, INCLUDES the implicit World& and entt::entity parameters.
		// Note that if this event is bound to a non-static function, another
		// parameter, the reference to the component, precedes the mEventParams here.
		std::vector<TypeTraits> mEventParams{};

		// mOutputPin.mTypeTraits is equal to MakeTypeTraits<void>() if this function
		// has no return value.
		MetaFuncNamedParam mOutputPin{};

		// EventParams, EXCLUDES the implicit World& and entt::entity parameters.
		// These are the parameters shown to a user using visual scripting.
		std::vector<MetaFuncNamedParam> mInputPins{};
	};

	template<typename Derived, typename Ret, typename... Args, EventFlags Flags>
	class EventType<Derived, Ret(Args...), Flags> :
		public EventBase
	{
	public:
		template<typename... Pins>
		EventType(std::string_view name, Pins&&... pins);
	};

	//********************************//
	//				API				  //
	//********************************//

	/**
	* \brief Binds an event to a type.
	 * \tparam Class The class that is being reflected.
	 * \tparam Args The arguments of the events. Note that each event also requires the first argument to be Class& (see Class template argument).
	 * \param type A type that is currently being reflected.
	 * \param event The event you would like to bind the function to, such as sOnTick.
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
	 *	CE::MetaType CE::EventTestingComponent::Reflect()
	 *	{
	 *		MetaType type = MetaType{ MetaType::T<EventTestingComponent>{}, "EventTestingComponent" };
	 *
	 *		BindEvent(type, sOnTick, &EventTestingComponent::OnTick);
	 *
	 *		ReflectComponentType<EventTestingComponent>(type);
	 *		return type;
	 *	}
	 */
	template<typename Class, typename Func, typename Derived, typename Ret, typename... Args, EventFlags Flags>
	void BindEvent(MetaType& type, const EventType<Derived, Ret(Args...), Flags>& event, Func&& func);

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

	struct BoundEvent
	{
		std::reference_wrapper<const MetaType> mType;
		std::reference_wrapper<const MetaFunc> mFunc;
		bool mIsStatic{};
	};

	/**
	 * \brief Returns the event bound during BindEvent, if any.
	 *
	 *  Example: TryGetEvent(componentType, sOnFixedTick);
	 */
	template<typename EventT>
	std::optional<BoundEvent> TryGetEvent(const MetaType& fromType, const EventT& event);

	template <typename EventT>
	std::vector<BoundEvent> GetAllBoundEvents(const EventT& event);

	Span<std::reference_wrapper<const EventBase>> GetAllEvents();

	struct OnConstruct :
		EventType<OnConstruct>
	{
		OnConstruct() :
			EventType("OnConstruct")
		{
		}
	};
	/**
	 * \brief Called immediately when the component is constructed.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	inline const OnConstruct sOnConstruct{};

	struct OnBeginPlay :
		EventType<OnBeginPlay>
	{
		OnBeginPlay() :
			EventType("OnBeginPlay")
		{
		}
	};
	/**
	 * \brief Called when the world begins play, or if the component is created after the world has begun play. Called after OnConstruct.
	 *
	 * Note that the order of BeginPlay is undefined; if you add a prefab to a world that has already begun play, you have no guarantee
	 * during OnBeginPlay that each component of the prefab has been added to the entity.
	 *
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	inline const OnBeginPlay sOnBeginPlay{};

	struct OnTick :
		EventType<OnTick, void(float)>
	{
		OnTick() :
			EventType("OnTick", "DeltaTime")
		{
		}

#ifdef EDITOR
		void OnDetailsInspect(ScriptFunc& scriptFunc) const override;
#endif
	};
	/**
	 * \brief Called every frame.
	 * \World& The world this component is in. 
	 * \entt::entity The owner of this component. 
	 * \float The deltatime. 
	 */
	inline const OnTick sOnTick{};

	struct OnFixedTick :
		EventType<OnFixedTick, void(float)>
	{
		OnFixedTick() :
			EventType("OnFixedTick", "DeltaTime")
		{
		}

#ifdef EDITOR
		void OnDetailsInspect(ScriptFunc& scriptFunc) const override;
#endif
	};
	/**
	 * \brief Called every sOnFixedTickStepSize seconds.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 * \float The deltatime, always equal to sOnFixedTickStepSize.
	 */
	inline const OnFixedTick sOnFixedTick{};

	/**
	 * \brief The number of seconds between fixed ticks.
	 */
	static constexpr float sOnFixedTickStepSize = 0.2f;

	namespace Props
	{
		/**
		 * \brief Can be added to the OnTick or OnFixedTick event to indicate that the event should be called even when the game is paused
		 */
		static constexpr std::string_view sShouldTickWhilstPausedTag = "sShouldTickWhilstPausedTag";

		/**
		 * \brief Can be added to the OnTick or OnFixedTick event to indicate that the event should be called in the editor before begin play has been called.
		 */
		static constexpr std::string_view sShouldTickBeforeBeginPlayTag = "sShouldTickBeforeBeginPlayTag";
	}

	struct OnEndPlay :
		EventType<OnEndPlay>
	{
		OnEndPlay() :
			EventType("OnEndPlay")
		{
		}
	};
	/**
	 * \brief Called for each component when an entity is destroyed,
	 * or when an individual component was removed. Only called if
	 * the world has begun player.
	 *
	 * All EndPlay events are called before any of the component's C++ destructor is called.
	 * This means if you destroy an entity with a TransformComponent, you can assume you can
	 * still get that component in your EndPlay function, as the TransformComponent has
	 * not been destroyed yet.
	 *
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	inline const OnEndPlay sOnEndPlay{};

#ifdef EDITOR

	struct OnInspect :
		EventType<OnInspect, void(), EventFlags::NotScriptable>
	{
		OnInspect() :
			EventType("OnInspect")
		{
		}
	};
	/**
	 * \brief For custom inspect logic. You can make calls to ImGui from this event.
	 * \World& The world you are inspecting
	 * \entt::entity The selected entity that has this component AND requires inspecting
	 */
	inline const OnInspect sOnInspect{};

	struct OnDrawGizmo :
		EventType<OnDrawGizmo>
	{
		OnDrawGizmo() :
			EventType("OnDrawGizmo")
		{
		}
	};
	/**
	 * \brief Implement this event to draw a gizmo if the entity is selected.
	 * Gizmos are drawn only when the entity is selected.
	 * For example an explosion script could draw a sphere showing the explosion radius.
	 * \World& The world this component is in.
	 * \entt::entity The owner of this component.
	 */
	inline const OnDrawGizmo sOnDrawGizmo{};
#endif // EDITOR

	//********************************//
	//			Implementation		  //
	//********************************//

	namespace Internal
	{
		static constexpr std::string_view sIsEventProp = "IsEvent";

		/**
		 * \brief Can be used to check if the MetaFunc returned from TryGetEvent should be called with an instance of the component.
		 */
		static constexpr std::string_view sIsEventStaticTag = "sIsEventStaticTag";
	}

	template <typename Derived, typename Ret, typename ... Args, EventFlags Flags>
	template <typename ... Pins>
	EventType<Derived, Ret(Args...), Flags>::EventType(std::string_view name, Pins&&... pins) :
		EventBase(name, 
			Flags, 
			MakeTypeTraits<Ret>(), 
			std::vector<TypeTraits>{ MakeTypeTraits<World&>(), MakeTypeTraits<entt::entity>(), MakeTypeTraits<Args>()... }, 
			std::vector<std::string_view>{ std::forward<Pins>(pins)... })
	{
		static constexpr size_t numOfNamesRequires = sizeof...(Args) + !std::is_same_v<void, Ret>;
		static_assert(sizeof...(Pins) >= numOfNamesRequires, "Too little names were provided. Keep in mind that return values must also be named.");
		static_assert(sizeof...(Pins) <= numOfNamesRequires, "Too many names were provided.");
	}

	template<typename Class, typename Func, typename Derived, typename Ret, typename... Args, EventFlags Flags>
	void BindEvent(MetaType& type, const EventType<Derived, Ret(Args...), Flags>& event, Func&& func)
	{
		static constexpr bool isProvidedFuncStatic = std::is_invocable_v<decltype(func), World&, entt::entity, Args...>;
		static constexpr bool isProvidedFuncMember = std::is_invocable_v<decltype(func), Class&, World&, entt::entity, Args...>;
		static constexpr bool isComponentCompletelyEmpty = entt::component_traits<std::remove_const_t<Class>>::page_size == 0;

		static constexpr bool isStaticOrMember = isProvidedFuncStatic || isProvidedFuncMember;

		static_assert(isStaticOrMember, "The parameters of the provided function do not match that of the event.");
		static_assert(!isComponentCompletelyEmpty || isProvidedFuncStatic, "EnTT does not construct components that are completely empty. All functions of this component that you want to bind to an event must be static.");

		MetaFunc* eventFunc{};

		if constexpr (isProvidedFuncStatic)
		{
			eventFunc = &type.AddFunc(std::function<Ret(World&, entt::entity, Args...)>{ std::forward<Func>(func) }, event.mName);
			eventFunc->GetProperties().Add(Internal::sIsEventStaticTag);
		}
		else
		{
			ASSERT(type.GetTypeId() == MakeStrippedTypeId<Class>());
			eventFunc = &type.AddFunc(std::function<Ret(Class&, World&, entt::entity, Args...)>{ std::forward<Func>(func) }, event.mName);
		}
		eventFunc->GetProperties().Add(Internal::sIsEventProp);
	}

	template <typename FuncRet, typename FuncObj, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet (FuncObj::* func)(FuncParams...))
	{
		BindEvent<FuncObj>(type, event, func);
	}

	template <typename FuncRet, typename FuncObj, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet (FuncObj::* func)(FuncParams...) const)
	{
		BindEvent<const FuncObj>(type, event, func);
	}

	template <typename FuncRet, typename... FuncParams, typename EventT>
	void BindEvent(MetaType& type, const EventT& event, FuncRet (*func)(FuncParams...))
	{
		BindEvent<std::monostate>(type, event, func);
	}

	namespace Internal
	{
		std::optional<BoundEvent> TryGetEvent(const MetaType& fromType, std::string_view eventName);
	}

	template <typename EventT>
	std::optional<BoundEvent> TryGetEvent(const MetaType& fromType, const EventT& event)
	{
		return Internal::TryGetEvent(fromType, event.mName);
	}

	namespace Internal
	{
		std::vector<BoundEvent> GetAllBoundEvents(std::string_view eventName);
	}

	template <typename EventT>
	std::vector<BoundEvent> GetAllBoundEvents(const EventT& event)
	{
		return Internal::GetAllBoundEvents(event.mName);
	}
}
