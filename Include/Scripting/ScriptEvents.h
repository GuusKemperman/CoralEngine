#pragma once

#include "Assets/Script.h"
#include "Utilities/Events.h"

namespace CE
{
	class Script;
	class ScriptFunc;
	class MetaFunc;
	class MetaType;

	class ScriptEvent
	{
	public:
		template <typename Ret, typename... Args, bool IsAlwaysStatic>
		ScriptEvent(const Event<Ret(Args...), IsAlwaysStatic>& event, std::vector<MetaFuncNamedParam>&& params,
		            std::optional<MetaFuncNamedParam>&& ret);

		MetaFunc& Declare(TypeId selfTypeId, MetaType& toType) const;
		void Define(MetaFunc& metaFunc, const ScriptFunc& scriptFunc,
		            const std::shared_ptr<const Script>& script) const;

		/**
		 * \brief Can be used to make calls to ImGui so the user can add/remove event specific properties.
		 */
		virtual void OnDetailsInspect([[maybe_unused]] ScriptFunc& scriptFunc) const {}
		
		std::reference_wrapper<const EventBase> mBasedOnEvent;

		std::vector<MetaFuncNamedParam> mParamsToShowToUser{};
		std::optional<MetaFuncNamedParam> mReturnValueToShowToUser{};

	private:
		virtual MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc,
		                                           const std::shared_ptr<const Script>& script) const = 0;

		std::vector<TypeTraits> mEventParams{};
		TypeTraits mEventReturnType{};

		bool mIsStatic{};
	};

	class ScriptOnlyPassComponentEvent :
		public ScriptEvent
	{
	public:
		template<typename EventT>
		ScriptOnlyPassComponentEvent(const EventT& event) :
			ScriptEvent(event, {}, std::nullopt) {}

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc,
			const std::shared_ptr<const Script>& script) const override;
	};

	// For both Tick and FixedTick
	class ScriptTickEvent :
		public ScriptEvent
	{
	public:
		template<typename EventT>
		ScriptTickEvent(const EventT& event) :
			ScriptEvent(event, { { MakeTypeTraits<float>(), "DeltaTime" } }, std::nullopt) {}

		void OnDetailsInspect(ScriptFunc& scriptFunc) const override;

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc,
			const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptAITickEvent final :
		public ScriptEvent
	{
	public:
		ScriptAITickEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc,
		                                   const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptAIEvaluateEvent final :
		public ScriptEvent
	{
	public:
		ScriptAIEvaluateEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc,
		                                   const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptAbilityActivateEvent final :
		public ScriptEvent
	{
	public:
		ScriptAbilityActivateEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc,
		                                   const std::shared_ptr<const Script>& script) const override;
	};

	class CollisionEvent :
		public ScriptEvent
	{
	public:
		template <typename EventT>
		CollisionEvent(const EventT& event);

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc,
		                                   const std::shared_ptr<const Script>& script) const override;
	};


	template <typename Ret, typename... Args, bool IsAlwaysStatic>
	ScriptEvent::ScriptEvent(const Event<Ret(Args...), IsAlwaysStatic>& event,
	                         std::vector<MetaFuncNamedParam>&& params, std::optional<MetaFuncNamedParam>&& ret) :
		mBasedOnEvent(event),
		mParamsToShowToUser(std::move(params)),
		mReturnValueToShowToUser(std::move(ret)),
		mEventParams({MakeTypeTraits<Args>()...}),
		mEventReturnType(MakeTypeTraits<Ret>()),
		mIsStatic(IsAlwaysStatic)
	{
	}

	template <typename EventT>
	CollisionEvent::CollisionEvent(const EventT& event) :
		ScriptEvent(event, {
			            {MakeTypeTraits<entt::entity>(), "Other"},
			            {MakeTypeTraits<float>(), "Depth"},
			            {MakeTypeTraits<glm::vec2>(), "Hit Normal"},
			            {MakeTypeTraits<glm::vec2>(), "Contact point"},
		            }, std::nullopt)
	{
	}

	static const ScriptOnlyPassComponentEvent sOnConstructScriptEvent{ sConstructEvent };
	static const ScriptOnlyPassComponentEvent sOnBeginPlayScriptEvent{ sBeginPlayEvent };
	static const ScriptTickEvent sOnTickScriptEvent{ sTickEvent };
	static const ScriptTickEvent sOnFixedTickScriptEvent{ sFixedTickEvent };
	static const ScriptOnlyPassComponentEvent sOnDestructScriptEvent{ sDestructEvent };
	static const ScriptOnlyPassComponentEvent sOnAIStateEnterScriptEvent{ sAIStateEnterEvent };
	static const ScriptAITickEvent sOnAITickScriptEvent{};
	static const ScriptOnlyPassComponentEvent sOnAIStateExitScriptEvent{ sAIStateExitEvent };
	static const ScriptAIEvaluateEvent sAIEvaluateScriptEvent{};
	static const ScriptAbilityActivateEvent sScriptAbilityActivateEvent{};
	static const CollisionEvent sOnCollisionEntryScriptEvent{ sCollisionEntryEvent };
	static const CollisionEvent sOnCollisionStayScriptEvent{ sCollisionStayEvent };
	static const CollisionEvent sOnCollisionExitScriptEvent{ sCollisionExitEvent };
	static const ScriptOnlyPassComponentEvent sOnButtonPressedScriptEvent{ sButtonPressEvent };

	static const std::array<std::reference_wrapper<const ScriptEvent>, 14> sAllScriptableEvents
	{
		sOnConstructScriptEvent,
		sOnDestructScriptEvent,
		sOnBeginPlayScriptEvent,
		sOnTickScriptEvent,
		sOnFixedTickScriptEvent,
		sOnAIStateEnterScriptEvent,
		sOnAITickScriptEvent,
		sOnAIStateExitScriptEvent,
		sAIEvaluateScriptEvent,
		sScriptAbilityActivateEvent,
		sOnCollisionEntryScriptEvent,
		sOnCollisionStayScriptEvent,
		sOnCollisionExitScriptEvent,
		sOnButtonPressedScriptEvent
	};
}
