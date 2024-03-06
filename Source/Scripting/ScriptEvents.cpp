#include "Precomp.h"
#include "Scripting/ScriptEvents.h"

#include "Core/VirtualMachine.h"
#include "Scripting/ScriptFunc.h"
#include "World/World.h"

Engine::ScriptEvent::ScriptEvent(const EventBase& event, std::vector<MetaFuncNamedParam>&& params, std::optional<MetaFuncNamedParam>&& ret) :
	mBasedOnEvent(event),
	mParamsToShowToUser(std::move(params)),
	mReturnValueToShowToUser(std::move(ret))
{
}

Engine::ScriptOnConstructEvent::ScriptOnConstructEvent() :
	ScriptEvent(sConstructEvent, {}, std::nullopt)
{
}

Engine::MetaFunc& Engine::ScriptOnConstructEvent::Declare(TypeTraits selfTraits, MetaType& toType) const
{
	static_assert(std::is_same_v<const Event<void(World&, entt::entity)>, decltype(sConstructEvent)>);
	std::vector<MetaFuncNamedParam> metaParams
	{
		{ selfTraits },
		{ MakeTypeTraits<World&>() },
		{ MakeTypeTraits<entt::entity>() },
	};
	MetaFuncNamedParam metaReturn{ MakeTypeTraits<void>() };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mBasedOnEvent.get().mName,
		metaReturn,
		metaParams
	);

	func.GetProperties().Add(Internal::sIsEventProp);

	return func;
}

void Engine::ScriptOnConstructEvent::Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc,
	std::shared_ptr<const Script> script) const
{
	declaredFunc.RedirectFunction([&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			World& world = *args[1].As<World>();
			World::PushWorld(world);

			Span<MetaAny, 1> scriptArgs{ &args[0], 1 };
			FuncResult result = VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);

			World::PopWorld();
			return result;
		});
}

Engine::ScriptOnBeginPlayEvent::ScriptOnBeginPlayEvent() :
	ScriptEvent(sBeginPlayEvent, {}, std::nullopt)
{
}

Engine::MetaFunc& Engine::ScriptOnBeginPlayEvent::Declare(TypeTraits selfTraits, MetaType& toType) const
{
	static_assert(std::is_same_v<const Event<void(World&, entt::entity)>, decltype(sBeginPlayEvent)>);
	std::vector<MetaFuncNamedParam> metaParams
	{
		{ selfTraits },
		{ MakeTypeTraits<World&>() },
		{ MakeTypeTraits<entt::entity>() },
	};

	MetaFuncNamedParam metaReturn{ MakeTypeTraits<void>() };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mBasedOnEvent.get().mName,
		metaReturn,
		metaParams
	);

	func.GetProperties().Add(Internal::sIsEventProp);

	return func;
}

void Engine::ScriptOnBeginPlayEvent::Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc,
	std::shared_ptr<const Script> script) const
{
	declaredFunc.RedirectFunction([&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			World& world = *args[1].As<World>();
			World::PushWorld(world);

			// The component already has the world
			// and it's owner
			Span<MetaAny, 1> scriptArgs{ &args[0], 1 };
			FuncResult result = VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);

			World::PopWorld();
			return result;
		});
}

Engine::ScriptTickEvent::ScriptTickEvent() :
	ScriptEvent(sTickEvent, { { MakeTypeTraits<float>(), "DeltaTime" } }, std::nullopt)
{
}

Engine::MetaFunc& Engine::ScriptTickEvent::Declare(TypeTraits selfTraits, MetaType& toType) const
{
	static_assert(std::is_same_v<const Event<void(World&, entt::entity, float)>, decltype(sTickEvent)>);
	std::vector<MetaFuncNamedParam> metaParams
	{
		{ selfTraits },
		{ MakeTypeTraits<World&>() },
		{ MakeTypeTraits<entt::entity>() },
		{ MakeTypeTraits<float>() },
	};

	MetaFuncNamedParam metaReturn{ MakeTypeTraits<void>() };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mBasedOnEvent.get().mName,
		metaReturn,
		metaParams
	);

	func.GetProperties().Add(Internal::sIsEventProp);

	return func;
}

void Engine::ScriptTickEvent::Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc, std::shared_ptr<const Script> script) const
{
	declaredFunc.RedirectFunction([&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The reference to the component and the deltatime
			std::array<MetaAny, 2> scriptArgs{ std::move(args[0]), std::move(args[3]) };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		});
}

Engine::ScriptFixedTickEvent::ScriptFixedTickEvent() :
	ScriptEvent(sFixedTickEvent, { { MakeTypeTraits<float>(), "DeltaTime" } }, std::nullopt)
{
}

Engine::MetaFunc& Engine::ScriptFixedTickEvent::Declare(TypeTraits selfTraits, MetaType& toType) const
{
	static_assert(std::is_same_v<const Event<void(World&, entt::entity, float)>, decltype(sTickEvent)>);
	std::vector<MetaFuncNamedParam> metaParams
	{
		{ selfTraits },
		{ MakeTypeTraits<World&>() },
		{ MakeTypeTraits<entt::entity>() },
	};

	MetaFuncNamedParam metaReturn{ MakeTypeTraits<void>() };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mBasedOnEvent.get().mName,
		metaReturn,
		metaParams
	);

	func.GetProperties().Add(Internal::sIsEventProp);

	return func;
}

void Engine::ScriptFixedTickEvent::Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc,
	std::shared_ptr<const Script> script) const
{
	declaredFunc.RedirectFunction([&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The reference to the component and the deltatime
			std::array<MetaAny, 2> scriptArgs{ std::move(args[0]), MetaAny{ sFixedTickEventStepSize } };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		});
}

Engine::ScriptAITickEvent::ScriptAITickEvent() :
	ScriptEvent(sAITickEvent, { { MakeTypeTraits<float>(), "DeltaTime" } }, std::nullopt)
{
}

Engine::MetaFunc& Engine::ScriptAITickEvent::Declare(TypeTraits selfTraits, MetaType& toType) const
{
	static_assert(std::is_same_v<const Event<void(World&, entt::entity, float)>, decltype(sAITickEvent)>);

	std::vector<MetaFuncNamedParam> metaParams
	{
		{ selfTraits },
		{ MakeTypeTraits<World&>() },
		{ MakeTypeTraits<entt::entity>() },
		{ MakeTypeTraits<float>() },
	};

	MetaFuncNamedParam metaReturn{ MakeTypeTraits<void>() };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mBasedOnEvent.get().mName,
		metaReturn,
		metaParams
	);

	func.GetProperties().Add(Internal::sIsEventProp);

	return func;
}

void Engine::ScriptAITickEvent::Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc,
	std::shared_ptr<const Script> script) const
{
	declaredFunc.RedirectFunction([&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The reference to the component and the deltatime
			std::array<MetaAny, 2> scriptArgs{ std::move(args[0]), std::move(args[3]) };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		});
}

Engine::ScriptAIEvaluateEvent::ScriptAIEvaluateEvent() :
	ScriptEvent(sAIEvaluateEvent, {}, MetaFuncNamedParam{ MakeTypeTraits<float>(), "Score" })
{
}

Engine::MetaFunc& Engine::ScriptAIEvaluateEvent::Declare(TypeTraits selfTraits, MetaType& toType) const
{
	static_assert(std::is_same_v<const Event<float(const World&, entt::entity), true>, decltype(sAIEvaluateEvent)>);

	selfTraits.mForm = TypeForm::ConstRef;

	std::vector<MetaFuncNamedParam> metaParams
	{
		{ selfTraits },
		{ MakeTypeTraits<const World&>() },
		{ MakeTypeTraits<entt::entity>() },
	};

	MetaFuncNamedParam metaReturn{ MakeTypeTraits<float>() };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mBasedOnEvent.get().mName,
		metaReturn,
		metaParams
	);

	func.GetProperties().Add(Internal::sIsEventProp);
	func.GetProperties().Set(Props::sIsScriptPure, true);

	return func;
}

void Engine::ScriptAIEvaluateEvent::Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc,
	std::shared_ptr<const Script> script) const
{
	declaredFunc.RedirectFunction([&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The component already has the world
			// and it's owner
			Span<MetaAny, 0> scriptArgs{};
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		});
}

Engine::ScriptAbilityActivateEvent::ScriptAbilityActivateEvent() :
	ScriptEvent(sAbilityActivateEvent, { { MakeTypeTraits<entt::entity>(), "Ability user" } }, std::nullopt)
{
}

Engine::MetaFunc& Engine::ScriptAbilityActivateEvent::Declare(TypeTraits, MetaType& toType) const
{
	static_assert(std::is_same_v<const Event<void(World&, entt::entity), false, true>, decltype(sAbilityActivateEvent)>);

	std::vector<MetaFuncNamedParam> metaParams
	{
		{ MakeTypeTraits<World&>() },
		{ MakeTypeTraits<entt::entity>() },
	};

	MetaFuncNamedParam metaReturn{ MakeTypeTraits<void>() };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mBasedOnEvent.get().mName,
		metaReturn,
		metaParams
	);

	func.GetProperties().Add(Internal::sIsEventProp).Add(Props::sIsEventStaticTag);

	return func;
}

void Engine::ScriptAbilityActivateEvent::Define(MetaFunc& declaredFunc, const ScriptFunc& scriptFunc,
	std::shared_ptr<const Script> script) const
{
	declaredFunc.RedirectFunction([&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The script knows about world, but we do have to provide entt::entity
			Span<MetaAny, 1> scriptArgs{ &args[1], 1 };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		});
}
