#include "Precomp.h"
#include "Scripting/Nodes/MetaFieldScriptNode.h"

#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "GSON/GSONBinary.h"
#include "Scripting/ScriptTools.h"

CE::NodeInvolvingField::NodeInvolvingField(const ScriptNodeType type, 
	ScriptFunc& scriptFunc, 
	const MetaField& field) :
	FunctionLikeNode(type, scriptFunc),
	mTypeName(field.GetOuterType().GetName()),
	mFieldName(field.GetName())
{
}

void CE::NodeInvolvingField::CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const
{
	FunctionLikeNode::CollectErrors(inserter, scriptFunc);

	if (mCachedField == nullptr)
	{
		inserter = { ScriptError::Type::TypeCannotBeMember,  { scriptFunc, *this }, Format("Variable {} no longer exists in type {}", mFieldName, mTypeName) };
	}
}

void CE::NodeInvolvingField::SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const
{
	ScriptNode::SerializeTo(to, scriptFunc);

	to.AddGSONMember("typeName") << mTypeName;
	to.AddGSONMember("memberName") << mFieldName;
}

bool CE::NodeInvolvingField::DeserializeVirtual(const BinaryGSONObject& from)
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
	*memberName >> mFieldName;

	return true;
}

void CE::NodeInvolvingField::PostDeclarationRefresh(ScriptFunc& scriptFunc)
{
	const MetaType* const metaType = MetaManager::Get().TryGetType(Name{ mTypeName });

	if (metaType == nullptr)
	{
		FunctionLikeNode::PostDeclarationRefresh(scriptFunc);
		return;
	}

	// In case our type got renamed
	mTypeName = metaType->GetName();
	mCachedField = metaType->TryGetField(Name{ mFieldName });
}

CE::SetterScriptNode::SetterScriptNode(ScriptFunc& scriptFunc, const MetaField& field):
	NodeInvolvingField(ScriptNodeType::Setter, scriptFunc, field)
{
	ASSERT_LOG(CanBeSetThroughScripts(field),
	           "{}::{} cannot be set through scripts; Check using CanBeSetThroughScripts first",
	           field.GetOuterType().GetName(),
	           field.GetName());

	ConstructExpectedPins(scriptFunc);
}

void CE::SetterScriptNode::CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const
{
	NodeInvolvingField::CollectErrors(inserter, scriptFunc);

	if (mCachedField != nullptr
		&& !CanBeSetThroughScripts(*mCachedField))
	{
		inserter = { ScriptError::Type::UnderlyingFuncNoLongerExists, { scriptFunc, *this },
			Format("Variable {} in type {} cannot be set", mFieldName, mTypeName) };
	}
}

std::optional<CE::FunctionLikeNode::InputsOutputs> CE::SetterScriptNode::GetExpectedInputsOutputs(const ScriptFunc&) const
{
	const MetaField* const originalMemberData = TryGetOriginalField();

	if (originalMemberData == nullptr
		|| !CanBeSetThroughScripts(*originalMemberData))
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
	insOuts.mOutputs.emplace_back(GetSetterReturnType(*originalMemberData));

	return insOuts;
}

CE::GetterScriptNode::GetterScriptNode(ScriptFunc& scriptFunc, const MetaField& field, bool returnsCopy):
	NodeInvolvingField(ScriptNodeType::Getter, scriptFunc, field),
	mReturnsCopy(returnsCopy)
{
	ASSERT_LOG(CanBeGetThroughScripts(field, !returnsCopy),
		"{}::{} cannot be gotten through scripts; Check using CanBeSetThroughScripts first",
		field.GetOuterType().GetName(),
		field.GetName());

	ConstructExpectedPins(scriptFunc);
}

void CE::GetterScriptNode::CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const
{
	NodeInvolvingField::CollectErrors(inserter, scriptFunc);

	if (mCachedField != nullptr
		&& !CanBeGetThroughScripts(*mCachedField, !mReturnsCopy))
	{
		inserter = { ScriptError::Type::UnderlyingFuncNoLongerExists, { scriptFunc, *this },
			Format("Variable {} in type {} cannot be gotten", mFieldName, mTypeName) };
	}
}

std::string CE::GetterScriptNode::GetTitle(std::string_view memberName, bool returnsCopy)
{
	if (returnsCopy)
	{
		return Format("Get {} (a copy)", memberName);
	}
	return Format("Get {}", memberName);
}

void CE::GetterScriptNode::SerializeTo(BinaryGSONObject& to, const ScriptFunc& scriptFunc) const
{
	NodeInvolvingField::SerializeTo(to, scriptFunc);

	to.AddGSONMember("copy") << mReturnsCopy;
}

bool CE::GetterScriptNode::DeserializeVirtual(const BinaryGSONObject& from)
{
	if (!NodeInvolvingField::DeserializeVirtual(from))
	{
		return false;
	}

	const BinaryGSONMember* copy = from.TryGetGSONMember("copy");

	if (copy != nullptr)
	{
		*copy >> mReturnsCopy;
	}
	else
	{
		mReturnsCopy = true;
	}

	return true;
}

std::optional<CE::FunctionLikeNode::InputsOutputs> CE::GetterScriptNode::GetExpectedInputsOutputs(const ScriptFunc&) const
{
	const MetaField* const originalMemberData = TryGetOriginalField();

	if (originalMemberData == nullptr
		|| !CanBeGetThroughScripts(*originalMemberData, !mReturnsCopy))
	{
		return std::nullopt;
	}

	InputsOutputs insOuts{};

	// The target
	insOuts.mInputs.emplace_back(TypeTraits
		{
			originalMemberData->GetOuterType().GetTypeId(),
			mReturnsCopy ? TypeForm::ConstRef : TypeForm::Ref,
		}, mTypeName);

	insOuts.mOutputs.emplace_back(TypeTraits{ originalMemberData->GetType().GetTypeId(), mReturnsCopy ? TypeForm::Value : TypeForm::Ref });

	return insOuts;
}
