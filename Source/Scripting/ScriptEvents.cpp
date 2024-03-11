#include "Precomp.h"
#include "Scripting/ScriptEvents.h"

#include "Core/VirtualMachine.h"
#include "Scripting/ScriptFunc.h"
#include "World/World.h"

Engine::MetaFunc& Engine::ScriptEvent::Declare(TypeId selfTypeId, MetaType& toType) const
{
	std::vector<MetaFuncNamedParam> metaParams{};

	if (!mIsStatic)
	{
		metaParams.emplace_back(TypeTraits{ selfTypeId, mIsPure ? TypeForm::ConstRef : TypeForm::Ref });
	}
	metaParams.insert(metaParams.end(), mEventParams.begin(), mEventParams.end());

	MetaFuncNamedParam metaReturn{ mEventReturnType };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mBasedOnEvent.get().mName,
		std::move(metaReturn),
		std::move(metaParams)
	);

	func.GetProperties().Add(Internal::sIsEventProp).Set(Props::sIsScriptPure, mIsPure);

	return func;
}

void Engine::ScriptEvent::Define(MetaFunc& metaFunc, const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const
{
	return metaFunc.RedirectFunction(GetScriptInvoker(scriptFunc, script));
}

Engine::ScriptOnConstructEvent::ScriptOnConstructEvent() :
	ScriptEvent(sConstructEvent, {}, std::nullopt)
{
}

Engine::MetaFunc::InvokeT  Engine::ScriptOnConstructEvent::GetScriptInvoker(const ScriptFunc& scriptFunc,
	const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
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
		};
}

Engine::ScriptOnBeginPlayEvent::ScriptOnBeginPlayEvent() :
	ScriptEvent(sBeginPlayEvent, {}, std::nullopt)
{}

Engine::MetaFunc::InvokeT  Engine::ScriptOnBeginPlayEvent::GetScriptInvoker(const ScriptFunc& scriptFunc,
	const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
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
		};
}

Engine::ScriptTickEvent::ScriptTickEvent() :
	ScriptEvent(sTickEvent, { { MakeTypeTraits<float>(), "DeltaTime" } }, std::nullopt)
{
}

Engine::MetaFunc::InvokeT  Engine::ScriptTickEvent::GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The reference to the component and the deltatime
			std::array<MetaAny, 2> scriptArgs{ std::move(args[0]), std::move(args[3]) };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		};
}

Engine::ScriptFixedTickEvent::ScriptFixedTickEvent() :
	ScriptEvent(sFixedTickEvent, { { MakeTypeTraits<float>(), "DeltaTime" } }, std::nullopt)
{
}

Engine::MetaFunc::InvokeT  Engine::ScriptFixedTickEvent::GetScriptInvoker(const ScriptFunc& scriptFunc,
	const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The reference to the component and the deltatime
			std::array<MetaAny, 2> scriptArgs{ std::move(args[0]), MetaAny{ sFixedTickEventStepSize } };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		};
}

Engine::ScriptDestructEvent::ScriptDestructEvent() :
	ScriptEvent(sDestructEvent, {}, std::nullopt)
{
}

Engine::MetaFunc::InvokeT Engine::ScriptDestructEvent::GetScriptInvoker(const ScriptFunc& scriptFunc, const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
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
		};
}

Engine::ScriptAITickEvent::ScriptAITickEvent() :
	ScriptEvent(sAITickEvent, { { MakeTypeTraits<float>(), "DeltaTime" } }, std::nullopt)
{
}

Engine::MetaFunc::InvokeT  Engine::ScriptAITickEvent::GetScriptInvoker(const ScriptFunc& scriptFunc,
	const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The reference to the component and the deltatime
			std::array<MetaAny, 2> scriptArgs{ std::move(args[0]), std::move(args[3]) };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		};
}

Engine::ScriptAIEvaluateEvent::ScriptAIEvaluateEvent() :
	ScriptEvent(sAIEvaluateEvent, {}, MetaFuncNamedParam{ MakeTypeTraits<float>(), "Score" })
{
}

Engine::MetaFunc::InvokeT  Engine::ScriptAIEvaluateEvent::GetScriptInvoker(const ScriptFunc& scriptFunc,
	const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The component already has the world
			// and it's owner
			Span<MetaAny, 1> scriptArgs{ &args[0], 1 };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		};
}

Engine::ScriptAbilityActivateEvent::ScriptAbilityActivateEvent() :
	ScriptEvent(sAbilityActivateEvent, { { MakeTypeTraits<entt::entity>(), "Ability user" } }, std::nullopt)
{
}

Engine::MetaFunc::InvokeT  Engine::ScriptAbilityActivateEvent::GetScriptInvoker(const ScriptFunc& scriptFunc,
	const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			// The script knows about world, but we do have to provide entt::entity
			Span<MetaAny, 1> scriptArgs{ &args[1], 1 };
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		};
}

Engine::MetaFunc::InvokeT Engine::CollisionEvent::GetScriptInvoker(const ScriptFunc& scriptFunc,
	const std::shared_ptr<const Script>& script) const
{
	return [&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue()]
	(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			std::array<MetaAny, 5> scriptArgs{
				std::move(args[0]), // The instance of our component
				std::move(args[3]), // The other entity
				std::move(args[4]), // Depth
				std::move(args[5]), // Hit normal
				std::move(args[6]), // Contact point
			};
			return VirtualMachine::Get().ExecuteScriptFunction(scriptArgs, rvoBuffer, scriptFunc, firstNode, entry);
		};
}
