#include "Precomp.h"
#include "Scripting/Nodes/FunctionLikeScriptNode.h"

#include "Scripting/ScriptIds.h"
#include "Scripting/ScriptFunc.h"

void Engine::FunctionLikeNode::ConstructExpectedPins(ScriptFunc& scriptFunc)
{
	PostDeclarationRefresh(scriptFunc);

	std::optional<InputsOutputs> insOuts = GetExpectedInputsOutputs(scriptFunc);

	if (!insOuts.has_value())
	{
		LOG(LogScripting, Warning, "Cannot ConstructExpectedPins for {}: original function no longer exists", GetDisplayName());
		return;
	}

	SetPins(scriptFunc, std::move(*insOuts));
}


#ifdef REMOVE_FROM_SCRIPTS_ENABLED
void Engine::FunctionLikeNode::PostDeclarationRefresh(ScriptFunc& scriptFunc)
{
	const std::optional<InputsOutputs> insOuts = GetExpectedInputsOutputs(scriptFunc);
	
	if (!insOuts.has_value())
	{
		ScriptNode::PostDeclarationRefresh(scriptFunc);
		return;
	}

	RefreshByComparingPins(scriptFunc, insOuts->mInputs, insOuts->mOutputs);
	ScriptNode::PostDeclarationRefresh(scriptFunc);
}
#endif // REMOVE_FROM_SCRIPTS_ENABLED

void Engine::FunctionLikeNode::CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const
{
	ScriptNode::CollectErrors(inserter, scriptFunc);

	const std::optional<InputsOutputs> insOuts = GetExpectedInputsOutputs(scriptFunc);

	if (insOuts.has_value())
	{
		return GetErrorsByComparingPins(inserter, scriptFunc, insOuts->mInputs, insOuts->mOutputs);
	}

	inserter = { ScriptError::Type::UnderlyingFuncNoLongerExists,  { scriptFunc, *this } };
}
