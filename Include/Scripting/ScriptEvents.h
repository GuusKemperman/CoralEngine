#pragma once

#include "Assets/Script.h"
#include "Utilities/Events.h"

namespace Engine
{
	class Script;
	class ScriptFunc;
	class MetaFunc;
	class MetaType;

	class ScriptEvent
	{
	public:
		template<typename Ret, typename... Args, bool IsPure, bool IsAlwaysStatic>
		ScriptEvent(const Event<Ret(Args...), IsPure, IsAlwaysStatic>& event, std::vector<MetaFuncNamedParam>&& params, std::optional<MetaFuncNamedParam>&& ret);

		MetaFunc& Declare(TypeId selfTypeId, MetaType& toType) const;
		void Define(MetaFunc& metaFunc, const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const;

		std::reference_wrapper<const EventBase> mBasedOnEvent;

		std::vector<MetaFuncNamedParam> mParamsToShowToUser{};
		std::optional<MetaFuncNamedParam> mReturnValueToShowToUser{};

	private:
		virtual MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const = 0;

		std::vector<TypeTraits> mEventParams{};
		TypeTraits mEventReturnType{};

		bool mIsStatic{};
		bool mIsPure{};
	};

	class ScriptOnConstructEvent final :
		public ScriptEvent
	{
	public:
		ScriptOnConstructEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptOnBeginPlayEvent final :
		public ScriptEvent
	{
	public:
		ScriptOnBeginPlayEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptTickEvent final :
		public ScriptEvent
	{
	public:
		ScriptTickEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptFixedTickEvent final :
		public ScriptEvent
	{
	public:
		ScriptFixedTickEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptDestructEvent final :
		public ScriptEvent
	{
	public:
		ScriptDestructEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptAITickEvent final :
		public ScriptEvent
	{
	public:
		ScriptAITickEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptAIEvaluateEvent final :
		public ScriptEvent
	{
	public:
		ScriptAIEvaluateEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;
	};

	class ScriptAbilityActivateEvent final :
		public ScriptEvent
	{
	public:
		ScriptAbilityActivateEvent();

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;
	};

	class CollisionEvent :
		public ScriptEvent
	{
	public:
		template<typename EventT>
		CollisionEvent(const EventT& event);

	private:
		MetaFunc::InvokeT GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const override;

	};
	class ScriptCollisionEntryEvent final :
		public CollisionEvent
	{
	public:
		ScriptCollisionEntryEvent() : CollisionEvent(sCollisionEntryEvent) {}
	};

	class ScriptCollisionStayEvent final :
		public CollisionEvent
	{
	public:
		ScriptCollisionStayEvent() : CollisionEvent(sCollisionStayEvent) {}
	};

	class ScriptCollisionExitEvent final :
		public CollisionEvent
	{
	public:
		ScriptCollisionExitEvent() : CollisionEvent(sCollisionExitEvent) {}
	};

	template <typename Ret, typename ... Args, bool IsPure, bool IsAlwaysStatic>
	ScriptEvent::ScriptEvent(const Event<Ret(Args...), IsPure, IsAlwaysStatic>& event,
		std::vector<MetaFuncNamedParam>&& params, std::optional<MetaFuncNamedParam>&& ret) :
		mBasedOnEvent(event),
		mParamsToShowToUser(std::move(params)),
		mReturnValueToShowToUser(std::move(ret)),
		mEventParams({ MakeTypeTraits<Args>()... }),
		mEventReturnType(MakeTypeTraits<Ret>()),
		mIsStatic(IsAlwaysStatic),
		mIsPure(IsPure)
	{
	}

	template <typename EventT>
	CollisionEvent::CollisionEvent(const EventT& event) :
		ScriptEvent(event, {
			{ MakeTypeTraits<entt::entity>(), "Other" },
			{ MakeTypeTraits<float>(), "Depth" },
			{ MakeTypeTraits<glm::vec2>(), "Hit Normal" },
			{ MakeTypeTraits<glm::vec2>(), "Contact point" },
			}, std::nullopt)
	{
	}

	static const ScriptOnConstructEvent sOnConstructScriptEvent{};
	static const ScriptOnBeginPlayEvent sOnBeginPlayScriptEvent{};
	static const ScriptTickEvent sOnTickScriptEvent{};
	static const ScriptFixedTickEvent sOnFixedTickScriptEvent{};
	static const ScriptDestructEvent sOnDestructScriptEvent{};
	static const ScriptAITickEvent sAITickScriptEvent{};
	static const ScriptAIEvaluateEvent sAIEvaluateScriptEvent{};
	static const ScriptAbilityActivateEvent sScriptAbilityActivateEvent{};
	static const ScriptCollisionEntryEvent sOnCollisionEntryScriptEvent{};
	static const ScriptCollisionStayEvent sOnCollisionStayScriptEvent{};
	static const ScriptCollisionExitEvent sOnCollisionExitScriptEvent{};

	static const std::array<std::reference_wrapper<const ScriptEvent>, 11> sAllScriptableEvents
	{
		sOnConstructScriptEvent,
		sOnDestructScriptEvent,
		sOnBeginPlayScriptEvent,
		sOnTickScriptEvent,
		sOnFixedTickScriptEvent,
		sAITickScriptEvent,
		sAIEvaluateScriptEvent,
		sScriptAbilityActivateEvent,
		sOnCollisionEntryScriptEvent,
		sOnCollisionStayScriptEvent,
		sOnCollisionExitScriptEvent,
	};
}
