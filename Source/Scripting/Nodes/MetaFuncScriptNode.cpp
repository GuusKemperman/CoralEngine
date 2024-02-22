#include "Precomp.h"
#include "Scripting/Nodes/MetaFuncScriptNode.h"

#include "Meta/MetaFunc.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"
#include "Scripting/ScriptTools.h"
#include "GSON/GSONBinary.h"

Engine::MetaFuncScriptNode::MetaFuncScriptNode(ScriptFunc& scriptFunc, 
	const MetaType& typeWhichThisFuncIsFrom, 
	const MetaFunc& func) :
	FunctionLikeNode(ScriptNodeType::FunctionCall, scriptFunc),
	mTypeName(typeWhichThisFuncIsFrom.GetName()),
	mNameOrType(std::holds_alternative<std::string>(func.GetNameOrType())
		? decltype(mNameOrType){std::get<std::string>(func.GetNameOrType())} : std::get<OperatorType>(func.GetNameOrType()))
{
	ASSERT_LOG(CanFunctionBeTurnedIntoNode(func),
		"{} cannot be turned into a node; Check using CanFunctionBeTurnedIntoNode first",
		func.GetDesignerFriendlyName());

	ASSERT((std::holds_alternative<std::string>(func.GetNameOrType()) ?
		typeWhichThisFuncIsFrom.TryGetFunc(std::get<std::string>(func.GetNameOrType())) :
		typeWhichThisFuncIsFrom.TryGetFunc(std::get<OperatorType>(func.GetNameOrType()))) != nullptr);

	ConstructExpectedPins(scriptFunc);
}

std::string Engine::MetaFuncScriptNode::GetTitle() const
{
	const MetaType* const type = MetaManager::Get().TryGetType(mTypeName);
	const MetaFunc* originalFunc = TryGetOriginalFunc();

	std::string funcName{ MetaFunc::GetDesignerFriendlyName(mNameOrType) };

	if (originalFunc == nullptr
		|| originalFunc->GetParameters().empty()
		|| originalFunc->GetParameters()[0].mTypeTraits.mStrippedTypeId != type->GetTypeId())
	{
		return Format("{} ({})", funcName, mTypeName);
	}
	return funcName;
}

void Engine::MetaFuncScriptNode::SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const
{
	ScriptNode::SerializeTo(to, scriptFunc);

	to.AddGSONMember("typename") << mTypeName;

	const bool hasName = std::holds_alternative<std::string>(mNameOrType);

	if (hasName)
	{
		to.AddGSONMember("funcName") << std::get<std::string>(mNameOrType);
	}
	else
	{
		to.AddGSONMember("opType") << std::get<OperatorType>(mNameOrType);
	}
}

void Engine::MetaFuncScriptNode::PostDeclarationRefresh(ScriptFunc& scriptFunc)
{

	const MetaType* const type = MetaManager::Get().TryGetType(mTypeName);

	if (type == nullptr)
	{
		FunctionLikeNode::PostDeclarationRefresh(scriptFunc);
		return;
	}

	const std::variant<Name, OperatorType> key = std::holds_alternative<std::string>(mNameOrType) ?
		std::variant<Name, OperatorType>{Name{ std::get<std::string>(mNameOrType) }} :
		std::get<OperatorType>(mNameOrType);

	mCachedFunc = type->TryGetFunc(key);
	FunctionLikeNode::PostDeclarationRefresh(scriptFunc);
}

glm::vec4 Engine::MetaFuncScriptNode::GetHeaderColor(const ScriptFunc& scriptFunc) const
{
	if (std::holds_alternative<OperatorType>(mNameOrType))
	{
		return ScriptNode::GetHeaderColor(scriptFunc);
	}

	return IsPure(scriptFunc) ? glm::vec4{ 0.404, 0.537, 0.384, 1.0f } : glm::vec4{ 0.318, 0.475, 0.576, 1.0f };
}

bool Engine::MetaFuncScriptNode::DeserializeVirtual(const BinaryGSONObject& from)
{
	if (!ScriptNode::DeserializeVirtual(from))
	{
		UNLIKELY;
		return false;
	}

	const BinaryGSONMember* typeName = from.TryGetGSONMember("typename");
	const BinaryGSONMember* funcName = from.TryGetGSONMember("funcName");
	const BinaryGSONMember* opType = from.TryGetGSONMember("opType");

	if (typeName == nullptr
		|| (funcName == nullptr && opType == nullptr)
		|| (funcName != nullptr && opType != nullptr))
	{
		UNLIKELY;
		LOG_TRIVIAL(LogAssets, Warning, "Could not deserialize MetaFuncScriptNode: missing values or ambiguity between name or type");
		return false;
	}

	*typeName >> mTypeName;

	if (funcName != nullptr)
	{
		std::string tmp{};
		*funcName >> tmp;
		mNameOrType = tmp;
	}
	else
	{
		OperatorType tmp{};
		*opType >> tmp;
		mNameOrType = tmp;
	}

	return true;
}

std::optional<Engine::FunctionLikeNode::InputsOutputs> Engine::MetaFuncScriptNode::GetExpectedInputsOutputs(const ScriptFunc&) const
{
	const MetaFunc* const originalFunc = TryGetOriginalFunc();

	if (originalFunc == nullptr)
	{
		return std::nullopt;
	}

	InputsOutputs insOuts{};

	if (!IsFunctionPure(*originalFunc))
	{
		insOuts.mInputs.emplace_back(ScriptPin::sFlow);
		insOuts.mOutputs.emplace_back(ScriptPin::sFlow);
	}

	const std::vector<MetaFuncNamedParam>& params = originalFunc->GetParameters();

	insOuts.mInputs.insert(insOuts.mInputs.end(), params.begin(), params.end());

	const MetaFuncNamedParam& returnValue = originalFunc->GetReturnType();

	if (returnValue.mTypeTraits != MakeTypeTraits<void>())
	{
		insOuts.mOutputs.emplace_back(returnValue.mTypeTraits, returnValue.mName);
	}

	return insOuts;
}
