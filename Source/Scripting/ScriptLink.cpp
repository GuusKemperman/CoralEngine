#include "Precomp.h"
#include "Scripting/ScriptLink.h"

#include "Scripting/ScriptPin.h"
#include "Scripting/ScriptTools.h"
#include "Scripting/ScriptFunc.h"
#include "Scripting/Nodes/ReroutScriptNode.h"
#include "GSON/GSONBinary.h"

CE::ScriptLink::ScriptLink(LinkId id, const ScriptPin& pinA, const ScriptPin& pinB) :
	mId(id),
	mInput(pinA.IsInput() ? pinA.GetId() : pinB.GetId()),
	mOutput(pinA.IsOutput() ? pinA.GetId() : pinB.GetId())
{
	ASSERT(CanCreateLink(pinA, pinB));
}

void CE::ScriptLink::SerializeTo(BinaryGSONObject& dest) const
{
	static_assert(std::is_trivially_copyable_v<LinkId>);
	dest.AddGSONMember("id") << mId.Get();
	dest.AddGSONMember("input") << mInput.Get();
	dest.AddGSONMember("output") << mOutput.Get();
}

std::optional<CE::ScriptLink> CE::ScriptLink::DeserializeFrom(const BinaryGSONObject& src)
{
	const BinaryGSONMember* id = src.TryGetGSONMember("id");
	const BinaryGSONMember* input = src.TryGetGSONMember("input");
	const BinaryGSONMember* output = src.TryGetGSONMember("output");

	if (id == nullptr
		|| input == nullptr
		|| output == nullptr)
	{
		UNLIKELY;
		LOG(LogAssets, Warning, "Failed to deserialize link: values missing");
		return std::nullopt;
	}

	ScriptLink link{};

	LinkId::ValueType tmp{};
	*id >> tmp;
	link.mId = tmp;

	*input >> tmp;
	link.mInput = tmp;

	*output >> tmp;
	link.mOutput = tmp;

	return link;
}

void CE::ScriptLink::CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const
{
	const ScriptPin* const inputPin = scriptFunc.TryGetPin(mInput);
	const ScriptPin* const outputPin = scriptFunc.TryGetPin(mOutput);

	if (inputPin == nullptr
		|| outputPin == nullptr)
	{
		inserter = { ScriptError::Type::CompilerBug, { scriptFunc, *this }, "One of the pins this node links to does not exist anymore!" };
		return;
	}

	if (!CanCreateLink(*inputPin, *outputPin))
	{
		inserter = { ScriptError::Type::LinkNotAllowed, { scriptFunc, *this } };
	}

	if (inputPin->IsOutput())
	{
		inserter = { ScriptError::Type::LinkNotAllowed, { scriptFunc, *this },
			"This link is not connected to an input pin - The input pin is actually an output pin?" };
	}

	if (outputPin->IsInput())
	{
		inserter = { ScriptError::Type::LinkNotAllowed, { scriptFunc, *this },
			"This link is not connected to an output pin - The output pin is actually an input pin?" };
	}

	const uint32 numOfLinksToInput = static_cast<uint32>(scriptFunc.GetAllLinksConnectedToPin(inputPin->GetId()).size());
	const uint32 numOfLinksToOutput = static_cast<uint32>(scriptFunc.GetAllLinksConnectedToPin(outputPin->GetId()).size());

	if (numOfLinksToInput == 0
		|| numOfLinksToOutput == 0)
	{
		/*VERY*/ UNLIKELY;
	
		inserter = { ScriptError::Type::CompilerBug, { scriptFunc, *this },
			"We are connected to a pin, but the pin is not connected to us? How is that even possible?" };

		return;
	}

	if (numOfLinksToInput > inputPin->HowManyLinksCanBeConnectedToThisPin())
	{
		inserter = { ScriptError::Type::LinkNotAllowed, { scriptFunc, *this },
			Format("Too many links are connected to the input pin {}", inputPin->GetName()) };
	}

	if (numOfLinksToOutput > outputPin->HowManyLinksCanBeConnectedToThisPin())
	{
		inserter = { ScriptError::Type::LinkNotAllowed, { scriptFunc, *this },
			Format("Too many links are connected to the output pin {}", outputPin->GetName()) };
	}
}

void CE::ScriptLink::GetReferencesToIds(ScriptIdInserter inserter)
{
	inserter = mId;
	inserter = mInput;
	inserter = mOutput;
}

