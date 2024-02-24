#include "Precomp.h"
#include "Scripting/Nodes/MetaMemberScriptNode.h"

#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "GSON/GSONBinary.h"
#include "Scripting/ScriptTools.h"

Engine::NodeInvolvingMetaMember::NodeInvolvingMetaMember(const ScriptNodeType type, 
	ScriptFunc& scriptFunc, 
	const MetaField& field) :
	FunctionLikeNode(type, scriptFunc),
	mTypeName(field.GetOuterType().GetName()),
	mMemberName(field.GetName())
{
	if (type == ScriptNodeType::Setter)
	{
		ASSERT_LOG(CanBeSetThroughScripts(field),
			"{}::{} cannot be set through scripts; Check using CanBeSetThroughScripts first",
			field.GetOuterType().GetName(),
			field.GetName());
	}
	else
	{
		ASSERT_LOG(CanBeGetThroughScripts(field),
			"{}::{} cannot be gotten through scripts; Check using CanBeSetThroughScripts first",
			field.GetOuterType().GetName(),
			field.GetName());
	}
}

void Engine::NodeInvolvingMetaMember::SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const
{
	ScriptNode::SerializeTo(to, scriptFunc);

	to.AddGSONMember("typeName") << mTypeName;
	to.AddGSONMember("memberName") << mMemberName;
}

bool Engine::NodeInvolvingMetaMember::DeserializeVirtual(const BinaryGSONObject& from)
{
	if (!ScriptNode::DeserializeVirtual(from))
	{
		UNLIKELY;
		return false;
	}

	const BinaryGSONMember* typeName = from.TryGetGSONMember("typeName");
	const BinaryGSONMember* memberName = from.TryGetGSONMember("memberName");

	if (typeName == nullptr
		|| memberName == nullptr)
	{
		UNLIKELY;
		LOG(LogAssets, Warning, "Failed to deserialize setter getter: missing values");
		return false;
	}

	*typeName >> mTypeName;
	*memberName >> mMemberName;

	return true;
}

void Engine::NodeInvolvingMetaMember::PostDeclarationRefresh(ScriptFunc& scriptFunc)
{
	const MetaType* const metaType = MetaManager::Get().TryGetType(Name{ mTypeName });
	mCachedField = metaType == nullptr ? nullptr : metaType->TryGetField(Name{ mMemberName });
	FunctionLikeNode::PostDeclarationRefresh(scriptFunc);
}

std::optional<Engine::FunctionLikeNode::InputsOutputs> Engine::SetterScriptNode::GetExpectedInputsOutputs(const ScriptFunc&) const
{
	const MetaField* const originalMemberData = TryGetOriginalMemberData();

	if (originalMemberData == nullptr)
	{
		return std::nullopt;
	}

	InputsOutputs insOuts{};

	insOuts.mInputs.emplace_back(ScriptPin::sFlow);
	
	// The target
	insOuts.mInputs.emplace_back(TypeTraits
		{
			originalMemberData->GetOuterType().GetTypeId(),
			TypeForm::Ref
		}, mTypeName);


	// The value we'll set it to
	insOuts.mInputs.emplace_back(
		TypeTraits
		{
			originalMemberData->GetType().GetTypeId(), TypeForm::ConstRef
		});
	
	insOuts.mOutputs.emplace_back(ScriptPin::sFlow);
	insOuts.mOutputs.emplace_back(TypeTraits{ originalMemberData->GetType().GetTypeId(), TypeForm::Value });

	return insOuts;
}

std::optional<Engine::FunctionLikeNode::InputsOutputs> Engine::GetterScriptNode::GetExpectedInputsOutputs(const ScriptFunc&) const
{
	const MetaField* const originalMemberData = TryGetOriginalMemberData();

	if (originalMemberData == nullptr)
	{
		return std::nullopt;
	}

	InputsOutputs insOuts{};

	// The target
	insOuts.mInputs.emplace_back(TypeTraits
		{
			originalMemberData->GetOuterType().GetTypeId(),
			TypeForm::ConstRef
		}, mTypeName);

	insOuts.mOutputs.emplace_back(TypeTraits{ originalMemberData->GetType().GetTypeId(), TypeForm::Value });

	return insOuts;
}
