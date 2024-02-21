#include "Precomp.h"
#include "Scripting/Nodes/EntryAndReturnScriptNode.h"

#include "Scripting/ScriptFunc.h"
#include "Assets/Script.h"
#include "GSON/GSONBinary.h"
#include "Core/AssetManager.h"

Engine::Internal::NodeInvolvingScriptFunc::NodeInvolvingScriptFunc(const ScriptNodeType type, 
	ScriptFunc& scriptFunc, 
	const Script& scriptThisFuncIsFrom) :
	FunctionLikeNode(type, scriptFunc),
	mNameOfScriptAsset(scriptThisFuncIsFrom.GetName()),
	mNameOfFunction(scriptFunc.GetName())
{
}

void Engine::Internal::NodeInvolvingScriptFunc::SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const
{
	ScriptNode::SerializeTo(to, scriptFunc);

	to.AddGSONMember("assetName") << mNameOfScriptAsset;
	to.AddGSONMember("funcName") << mNameOfFunction;
}

#ifdef REMOVE_FROM_SCRIPTS_ENABLED
void Engine::Internal::NodeInvolvingScriptFunc::PostDeclarationRefresh(ScriptFunc& scriptFunc)
{
	mNameOfFunction = scriptFunc.GetName();

	FunctionLikeNode::PostDeclarationRefresh(scriptFunc);
}
#endif // REMOVE_FROM_SCRIPTS_ENABLED

const Engine::ScriptFunc* Engine::Internal::NodeInvolvingScriptFunc::TryGetOriginalFunc() const
{
	const std::shared_ptr<const Script> script = AssetManager::Get().TryGetAsset<Script>(mNameOfScriptAsset);
	
	return script == nullptr ? nullptr : script->TryGetFunc(mNameOfFunction);
}

bool Engine::Internal::NodeInvolvingScriptFunc::DeserializeVirtual(const BinaryGSONObject& from)
{
	if (!ScriptNode::DeserializeVirtual(from))
	{
		UNLIKELY;
		return false;
	}

	const BinaryGSONMember* nameOfScriptAsset = from.TryGetGSONMember("assetName");
	const BinaryGSONMember* nameOfFunction = from.TryGetGSONMember("funcName");

	if (nameOfScriptAsset == nullptr
		|| nameOfFunction == nullptr)
	{
		UNLIKELY;
		LOG(LogAssets, Warning, "Failed to deserialize funcEntryNode: missing values");
		return false;
	}

	*nameOfScriptAsset >> mNameOfScriptAsset;
	*nameOfFunction >> mNameOfFunction;

	return true;
}

std::optional<Engine::FunctionLikeNode::InputsOutputs> Engine::FunctionEntryScriptNode::GetExpectedInputsOutputs(const ScriptFunc& scriptFunc) const
{
	InputsOutputs insOuts{};

	if (!scriptFunc.IsPure())
	{
		insOuts.mOutputs.emplace_back(ScriptPin::sFlow);
	}

	std::vector<ScriptVariableTypeData> params = scriptFunc.GetParameters(true);
	insOuts.mOutputs.insert(insOuts.mOutputs.end(), std::make_move_iterator(params.begin()), std::make_move_iterator(params.end()));

	return insOuts;
}

std::optional<Engine::FunctionLikeNode::InputsOutputs> Engine::FunctionReturnScriptNode::GetExpectedInputsOutputs(const ScriptFunc& scriptFunc) const
{
	InputsOutputs insOuts{};

	if (!scriptFunc.IsPure())
	{
		insOuts.mInputs.emplace_back(ScriptPin::sFlow);
	}

	if (scriptFunc.GetReturnType().has_value())
	{
		insOuts.mInputs.emplace_back(*scriptFunc.GetReturnType());
	}

	return insOuts;
}
