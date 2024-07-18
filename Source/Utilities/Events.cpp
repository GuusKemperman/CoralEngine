#include "Precomp.h"
#include "Utilities/Events.h"

#include "Core/VirtualMachine.h"
#include "Meta/MetaTools.h"
#include "Scripting/ScriptFunc.h"
#include "World/World.h"

std::optional<CE::BoundEvent> CE::TryGetEvent(const MetaType& fromType, const EventBase& base)
{
	const MetaFunc* func = fromType.TryGetFunc(base.mName);

	if (func != nullptr
		&& func->GetProperties().Has(Internal::sIsEventProp))
	{
		return BoundEvent{ fromType, *func, func->GetProperties().Has(Internal::sIsEventStaticTag) };
	}
	return std::nullopt;
}

std::vector<CE::BoundEvent> CE::GetAllBoundEventsSlow(const EventBase& base)
{
	std::vector<BoundEvent> bound{};

	for (const MetaType& type : MetaManager::Get().EachType())
	{
		std::optional<BoundEvent> boundEvent = TryGetEvent(type, base);

		if (!boundEvent.has_value())
		{
			continue;
		}

		bound.emplace_back(std::move(*boundEvent));
	}

	return bound;
}

namespace CE::Internal
{
	std::vector<std::reference_wrapper<const EventBase>>& GetEventsMutable()
	{
		static std::vector<std::reference_wrapper<const EventBase>> events{};
		return { events };
	}
}

CE::Span<std::reference_wrapper<const CE::EventBase>> CE::GetAllEvents()
{
	return { Internal::GetEventsMutable() };
}

CE::EventBase::EventBase(std::string_view name, EventFlags flags, TypeTraits eventReturnType,
	std::vector<TypeTraits>&& eventParams, std::vector<std::string_view>&& pinNames) :
	mName(name),
	mFlags(flags),
	mEventReturnType(eventReturnType),
	mEventParams(std::move(eventParams)),
	mOutputPin(eventReturnType, eventReturnType == MakeTypeTraits<void>() ? std::string_view{} : pinNames.back()),
	mInputPins([this, pinNames]
		{
			std::vector<MetaFuncNamedParam> params{};
			// World& and entt::entity are excluded in the input pins.
			params.reserve(mEventParams.size() - 2);

			for (size_t i = 0; i < mEventParams.size() - 2; i++)
			{
				params.emplace_back(mEventParams[i + 2], pinNames[i]);
			}

			return params;
		}())
{
	auto& allEvents = Internal::GetEventsMutable();

#ifdef ASSERTS_ENABLED
	{
		const auto existingEventWithSameName = std::find_if(allEvents.begin(), allEvents.end(),
			[name](const EventBase& other)
			{
				return other.mName == name;
			});
		ASSERT_LOG(existingEventWithSameName == allEvents.end(), "Multiple events with the name {}", name);
	}
#endif

	allEvents.emplace_back(*this);
}

CE::MetaFunc& CE::EventBase::Declare(TypeId selfTypeId, MetaType& toType) const
{
	std::vector<MetaFuncNamedParam> metaParams{ TypeTraits{ selfTypeId, TypeForm::Ref } };
	metaParams.insert(metaParams.end(), mEventParams.begin(), mEventParams.end());

	MetaFuncNamedParam metaReturn{ mEventReturnType };

	MetaFunc& func = toType.AddFunc([](MetaFunc::DynamicArgs, MetaFunc::RVOBuffer) -> FuncResult
		{
			return { "There were unresolved compilation errors" };
		},
		mName,
		std::move(metaReturn),
		std::move(metaParams)
	);

	func.GetProperties().Add(Internal::sIsEventProp);

	return func;
}

void CE::EventBase::Define(MetaFunc& metaFunc, const ScriptFunc& scriptFunc, const AssetHandle<Script>& script) const
{
	ASSERT(metaFunc.GetParameters().size() >= 3);

	// The component already has the world
	// and it's owner, so we don't pass those.
	const size_t numOfArgsToPass = metaFunc.GetParameters().size() - 2;

	metaFunc.RedirectFunction([&scriptFunc, script, firstNode = scriptFunc.GetFirstNode().GetValue(), entry = scriptFunc.GetEntryNode().GetValue(), numOfArgsToPass]
		(MetaFunc::DynamicArgs args, MetaFunc::RVOBuffer rvoBuffer) -> FuncResult
		{
			World& world = *args[1].As<World>();
			World::PushWorld(world);

			// Raii object that manages the lifetime
			struct ParamDeleter
			{
				~ParamDeleter()
				{
					for (size_t i = 0; i < mSize; i++)
					{
						mInputForms[i].~MetaAny();
					}
				}

				MetaAny* mInputForms;
				size_t mSize;
			};

			MetaAny* scriptArgs = static_cast<MetaAny*>(ENGINE_ALLOCA(numOfArgsToPass * sizeof(MetaAny)));
			ParamDeleter deleter{ scriptArgs, numOfArgsToPass };

			new (scriptArgs)MetaAny(MakeRef(args[0]));

			for (size_t i = 0; i < numOfArgsToPass - 1; i++)
			{
				new (&scriptArgs[i + 1])MetaAny(MakeRef(args[i + 3]));
			}

			FuncResult result = VirtualMachine::Get().ExecuteScriptFunction(Span<MetaAny>{ scriptArgs, numOfArgsToPass}, rvoBuffer, scriptFunc, firstNode, entry);

			World::PopWorld();
			return result;
		});
}

#ifdef EDITOR
namespace CE::Internal
{
	static void InspectOnTickEvent(ScriptFunc& scriptFunc)
	{
		MetaProps& props = scriptFunc.GetProps();

		bool tickWhilstPaused = props.Has(Props::sShouldTickWhilstPausedTag);

		if (ImGui::Checkbox("Tick while paused", &tickWhilstPaused))
		{
			if (tickWhilstPaused)
			{
				props.Add(Props::sShouldTickWhilstPausedTag);
			}
			else
			{
				props.Remove(Props::sShouldTickWhilstPausedTag);
			}
		}

		bool tickBeforeBeginPlay = props.Has(Props::sShouldTickBeforeBeginPlayTag);

		if (ImGui::Checkbox("Tick before begin play", &tickBeforeBeginPlay))
		{
			if (tickBeforeBeginPlay)
			{
				props.Add(Props::sShouldTickBeforeBeginPlayTag);
			}
			else
			{
				props.Remove(Props::sShouldTickBeforeBeginPlayTag);
			}
		}
	}
}

void CE::OnTick::OnDetailsInspect(ScriptFunc& scriptFunc) const
{
	Internal::InspectOnTickEvent(scriptFunc);
}

void CE::OnFixedTick::OnDetailsInspect(ScriptFunc& scriptFunc) const
{
	Internal::InspectOnTickEvent(scriptFunc);
}
#endif // EDITOR