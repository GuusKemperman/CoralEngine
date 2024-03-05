#include "Precomp.h"
#include "Scripting/ScriptEvents.h"

#include "Core/VirtualMachine.h"
#include "Scripting/ScriptFunc.h"

Engine::ScriptEvent::ScriptEvent(const EventBase& event, std::vector<MetaFuncNamedParam>&& params) :
	mBasedOnEvent(event),
	mParamsToShowToUser(std::move(params))
{
}

Engine::ScriptTickEvent::ScriptTickEvent() :
	ScriptEvent(sTickEvent, { { MakeTypeTraits<float>(), "DeltaTime" } })
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
	ScriptEvent(sFixedTickEvent, { { MakeTypeTraits<float>(), "DeltaTime" } })
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
