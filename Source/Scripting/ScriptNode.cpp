#include "Precomp.h"
#include "Scripting/ScriptNode.h"

#include "Assets/Script.h"
#include "Scripting/ScriptIds.h"
#include "Scripting/ScriptFunc.h"
#include "GSON/GSONBinary.h"
#include "Scripting/Nodes/ControlScriptNodes.h"
#include "Scripting/Nodes/CommentScriptNode.h"
#include "Scripting/Nodes/MetaFuncScriptNode.h"
#include "Scripting/Nodes/MetaMemberScriptNode.h"
#include "Scripting/Nodes/EntryAndReturnScriptNode.h"
#include "Scripting/Nodes/ReroutScriptNode.h"

Engine::ScriptNode::ScriptNode(const ScriptNodeType type) :
	mType(type)
{
}

Engine::ScriptNode::ScriptNode(const ScriptNodeType type, ScriptFunc& scriptFunc) :
	mId(static_cast<uint32>(scriptFunc.GetNumOfNodesIncludingRemoved() + 1)),
	mType(type)
{
}

void Engine::ScriptNode::CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const
{
	for (const ScriptPin& pin : GetPins(scriptFunc))
	{
		pin.CollectErrors(inserter, scriptFunc);
	}
}

void Engine::ScriptNode::SetPins(ScriptFunc& scriptFunc, InputsOutputs&& inputsOutputs)
{
#ifdef REMOVE_FROM_SCRIPTS_ENABLED
	ClearPins(scriptFunc);
#else 
	ASSERT(!mFirstPinId.IsValid() && mNumOfOutputs == 0 && mNumOfInputs)
#endif // REMOVE_FROM_SCRIPTS_ENABLED

	mNumOfInputs = static_cast<uint32>(inputsOutputs.mInputs.size());
	mNumOfOutputs = static_cast<uint32>(inputsOutputs.mOutputs.size());

	const uint32 total = mNumOfInputs + mNumOfOutputs;

	if (total == 0)
	{
		return;
	}

	Span<ScriptPin> pins = scriptFunc.AllocPins(mId, std::move(inputsOutputs.mInputs), std::move(inputsOutputs.mOutputs));
	mFirstPinId = pins[0].GetId();
}

#ifdef REMOVE_FROM_SCRIPTS_ENABLED
void Engine::ScriptNode::ClearPins(ScriptFunc& scriptFunc)
{
	if (mFirstPinId != PinId::Invalid)
	{
		scriptFunc.FreePins(mFirstPinId, mNumOfInputs + mNumOfOutputs);
	}
	mFirstPinId = PinId::Invalid;
	mNumOfInputs = 0;
	mNumOfOutputs = 0;
}


void Engine::ScriptNode::RefreshByComparingPins(ScriptFunc& scriptFunc,
	const std::vector<ScriptVariableTypeData>& expectedInputs,
	const std::vector<ScriptVariableTypeData>& expectedOutputs)
{
	CompareOutOfDataResult outOfDataResult = CheckIfOutOfDateByComparingPinType(scriptFunc, expectedInputs, expectedOutputs);

	if (outOfDataResult.mPinsThatMustBeUnlinkedBeforeWeCanRefresh.empty()
		&& (outOfDataResult.mAreOutputsOutOfDate || outOfDataResult.mAreInputsOutOfDate))
	{
		ClearPins(scriptFunc);
		SetPins(scriptFunc, { expectedInputs, expectedOutputs });

		outOfDataResult.mAreInputsOutOfDate = false;
		outOfDataResult.mAreOutputsOutOfDate = false;
	}

	// Update the names of the pins
	if (!outOfDataResult.mAreInputsOutOfDate)
	{
		auto inputs = GetInputs(scriptFunc);
		for (uint32 i = 0; i < inputs.size() && i < expectedInputs.size(); i++)
		{
			inputs[i].SetName(expectedInputs[i].GetName());
		}
	}

	// Update the names of the pins
	if (!outOfDataResult.mAreInputsOutOfDate)
	{
		auto outputs = GetOutputs(scriptFunc);
		for (uint32 i = 0; i < outputs.size() && i < expectedOutputs.size(); i++)
		{
			outputs[i].SetName(expectedOutputs[i].GetName());
		}
	}
}
#endif // REMOVE_FROM_SCRIPTS_ENABLED

bool Engine::ScriptNode::IsPure(const ScriptFunc& scriptFunc) const
{
	return mNumOfOutputs == 0 || !GetOutputs(scriptFunc)[0].IsFlow();
}

void Engine::ScriptNode::SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const
{
	to.AddGSONMember("type") << mType;
	to.AddGSONMember("id") << mId.Get();
	to.AddGSONMember("pos") << mPosition;

	BinaryGSONObject& inputs = to.AddGSONObject("inputs");
	for (const ScriptPin& input : GetInputs(scriptFunc))
	{
		input.SerializeTo(inputs.AddGSONObject(""));
	}

	BinaryGSONObject& outputs = to.AddGSONObject("outputs");
	for (const ScriptPin& output : GetOutputs(scriptFunc))
	{
		output.SerializeTo(outputs.AddGSONObject(""));
	}
}

std::unique_ptr<Engine::ScriptNode> Engine::ScriptNode::DeserializeFrom(const BinaryGSONObject& from,
	std::back_insert_iterator<std::vector<ScriptPin>> pinInserter, const uint32 version)
{
	const BinaryGSONMember* serializedType = from.TryGetGSONMember("type");
	const BinaryGSONObject* inputs = from.TryGetGSONObject("inputs");
	const BinaryGSONObject* outputs = from.TryGetGSONObject("outputs");

	if (serializedType == nullptr
		|| inputs == nullptr
		|| outputs == nullptr)
	{
		UNLIKELY;
		LOG(LogAssets, Warning, "Could not deserialize scriptnode: type, inputs or outputs were not serialized");
		return nullptr;
	}

	ScriptNodeType type{};
	*serializedType >> type;

	std::unique_ptr<ScriptNode> node{};

	switch (type)
	{
	case ScriptNodeType::Branch: node = std::make_unique<BranchScriptNode>(BranchScriptNode{}); break;
	case ScriptNodeType::Comment: node = std::make_unique<CommentScriptNode>(CommentScriptNode{}); break;
	case ScriptNodeType::FunctionCall: node = std::make_unique<MetaFuncScriptNode>(MetaFuncScriptNode{}); break;
	case ScriptNodeType::Setter: node = std::make_unique<SetterScriptNode>(SetterScriptNode{}); break;
	case ScriptNodeType::Getter: node = std::make_unique<GetterScriptNode>(GetterScriptNode{}); break;
	case ScriptNodeType::FunctionEntry: node = std::make_unique<FunctionEntryScriptNode>(FunctionEntryScriptNode{}); break;
	case ScriptNodeType::FunctionReturn: node = std::make_unique<FunctionReturnScriptNode>(FunctionReturnScriptNode{}); break;
	case ScriptNodeType::ForLoop: node = std::make_unique<ForLoopScriptNode>(ForLoopScriptNode{}); break;
	case ScriptNodeType::WhileLoop: node = std::make_unique<WhileLoopScriptNode>(WhileLoopScriptNode{}); break;
	case ScriptNodeType::Rerout: node = std::make_unique<RerouteScriptNode>(RerouteScriptNode{}); break;
	}

	if (node == nullptr)
	{
		LOG_FMT(LogAssets, Error, "Invalid node type {}", static_cast<uint32>(type));
		return node;
	}

	node->mType = type;

	if (!node->DeserializeVirtual(from))
	{
		return {};
	}

	for (const BinaryGSONObject& input : inputs->GetChildren())
	{
		std::optional<ScriptPin> deserializedInput = ScriptPin::DeserializeFrom(input, *node, version);

		if (!deserializedInput.has_value())
		{
			UNLIKELY;
			return {};
		}

		if (node->mFirstPinId == PinId::Invalid)
		{
			node->mFirstPinId = deserializedInput->GetId();
		}

		pinInserter = std::move(*deserializedInput);
		node->mNumOfInputs++;
	}

	for (const BinaryGSONObject& output : outputs->GetChildren())
	{
		std::optional<ScriptPin> deserializedOutput = ScriptPin::DeserializeFrom(output, *node, version);

		if (!deserializedOutput.has_value())
		{
			UNLIKELY;
			return {};
		}

		if (node->mFirstPinId == PinId::Invalid)
		{
			node->mFirstPinId = deserializedOutput->GetId();
		}

		pinInserter = std::move(*deserializedOutput);
		node->mNumOfOutputs++;

	}

	return node;
}

void Engine::ScriptNode::GetReferencesToIds(ScriptIdInserter inserter)
{
	inserter = mId;
	inserter = mFirstPinId;
}

bool Engine::ScriptNode::DeserializeVirtual(const BinaryGSONObject& from)
{
	const BinaryGSONMember* id = from.TryGetGSONMember("id");
	const BinaryGSONMember* pos = from.TryGetGSONMember("pos");

	if (id == nullptr
		|| pos == nullptr)
	{
		UNLIKELY;
		LOG(LogAssets, Warning, "Could not deserialize scriptnode: missing values");
		return false;
	}

	NodeId::ValueType tmpId{};
	*id >> tmpId;
	mId = tmpId;

	*pos >> mPosition;


	return true;
}

void Engine::ScriptNode::GetErrorsByComparingPins(ScriptErrorInserter inserter,
	const ScriptFunc& scriptFunc,
	const std::vector<ScriptVariableTypeData>& expectedInputs,
	const std::vector<ScriptVariableTypeData>& expectedOutputs) const
{
	const CompareOutOfDataResult outOfDataResult = CheckIfOutOfDateByComparingPinType(scriptFunc, expectedInputs, expectedOutputs);

	if (!outOfDataResult.mAreInputsOutOfDate && !outOfDataResult.mAreOutputsOutOfDate)
	{
		return;
	}

	inserter = { ScriptError::Type::NodeOutOfDate, { scriptFunc, *this } };

	for (const ScriptPin& pin : outOfDataResult.mPinsThatMustBeUnlinkedBeforeWeCanRefresh)
	{
		inserter = { ScriptError::Type::NodeOutOfDate, { scriptFunc, pin }, "Pin must be unlinked before node can be refreshed" };
	}
}

Engine::ScriptNode::CompareOutOfDataResult Engine::ScriptNode::CheckIfOutOfDateByComparingPinType(const ScriptFunc& scriptFunc,
	const std::vector<ScriptVariableTypeData>& expectedInputs,
	const std::vector<ScriptVariableTypeData>& expectedOutputs) const
{
	static constexpr auto compareToPins = [](const ScriptVariableTypeData& currentParams, const ScriptPin& oldPins) -> bool
		{
			return oldPins.GetTypeName() == currentParams.GetTypeName() && oldPins.GetTypeForm() == currentParams.GetTypeForm();
		};

	CompareOutOfDataResult result{};

	auto inputs = GetInputs(scriptFunc);
	auto outputs = GetOutputs(scriptFunc);
	result.mAreInputsOutOfDate = !std::equal(expectedInputs.begin(), expectedInputs.end(), inputs.begin(), inputs.end(), compareToPins);
	result.mAreOutputsOutOfDate = !std::equal(expectedOutputs.begin(), expectedOutputs.end(), outputs.begin(), outputs.end(), compareToPins);

	if (!result.mAreInputsOutOfDate && !result.mAreOutputsOutOfDate)
	{
		return result;
	}

	if (result.mAreInputsOutOfDate
		|| result.mAreOutputsOutOfDate)
	{
		for (const ScriptPin& pin : GetPins(scriptFunc))
		{
			if (pin.IsLinked())
			{
				result.mPinsThatMustBeUnlinkedBeforeWeCanRefresh.push_back(pin);
			}
		}
	}

	return result;
}

