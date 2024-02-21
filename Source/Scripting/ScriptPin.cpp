#include "Precomp.h"
#include "Scripting/ScriptPin.h"

#include "GSON/GSONBinary.h"
#include "Meta/MetaFunc.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Scripting/ScriptNode.h"
#include "Scripting/ScriptTools.h"

Engine::ScriptVariableTypeData::ScriptVariableTypeData(TypeTraits typeTraits, std::string_view name) :
	mName(name),
	mTypeForm(typeTraits.mForm)
{
	if (typeTraits == ScriptPin::sFlow)
	{
		mTypeName = std::string{ ScriptPin::sFlowDisplayName };
		mType = nullptr;
		return;
	}

	mType = MetaManager::Get().TryGetType(typeTraits.mStrippedTypeId);

	if (mType != nullptr)
	{
		mTypeName = mType->GetName();
	}
	else
	{
		UNLIKELY;
		LOG(LogScripting, Error, "Could not find type with typeId {} - Likely unintented", typeTraits.mStrippedTypeId);
	}
}

Engine::ScriptVariableTypeData::ScriptVariableTypeData(std::string_view typeName, TypeForm typeForm, std::string_view name) :
	mTypeName(typeName),
	mName(name),
	mType(MetaManager::Get().TryGetType(typeName)),
	mTypeForm(typeForm)
{
}

Engine::ScriptVariableTypeData::ScriptVariableTypeData(const MetaType& type, TypeForm typeForm, std::string_view name) :
	mTypeName(type.GetName()),
	mName(name),
	mType(&type),
	mTypeForm(typeForm)
{
}

bool Engine::ScriptVariableTypeData::IsFlow() const
{
	return mTypeName == ScriptPin::sFlowDisplayName;
}

void Engine::ScriptVariableTypeData::RefreshTypePointer()
{
	mType = MetaManager::Get().TryGetType(mTypeName);
	ASSERT(mType == nullptr || mType->GetName() == mTypeName);
}

Engine::ScriptPin::ScriptPin(PinId id, NodeId nodeId, ScriptPinKind kind,
                             const ScriptVariableTypeData& variableTypeInfo) :
	mId(id),
	mNodeId(nodeId),
	mParamInfo(std::move(variableTypeInfo)),
	mData(kind == ScriptPinKind::Output ? decltype(mData){OutputData{}} : InputData{})
{
}

uint32 Engine::ScriptPin::HowManyLinksCanBeConnectedToThisPin() const
{
	if (IsFlow())
	{
		if (IsOutput())
		{
			return 1;
		}
		else
		{
			return std::numeric_limits<uint32>::max();
		}
	}
	else
	{
		if (IsOutput())
		{
			return std::numeric_limits<uint32>::max();
		}
		else
		{
			return 1;
		}
	}
}

Engine::MetaAny* Engine::ScriptPin::TryGetValueIfNoInputLinked()
{
	if (!std::holds_alternative<InputData>(mData))
	{
		LOG(LogScripting, Warning, "Tried to get default value of non-input pin.");
		return nullptr;
	}

	InputData& inputData = std::get<InputData>(mData);

	if (inputData.mValueToUseIfNotLinked.index() == 1)
	{
		PostDeclarationRefresh();
		ASSERT(inputData.mValueToUseIfNotLinked.index() != 1 && "Value was not deserialized yet.");
	}

	if (inputData.mValueToUseIfNotLinked.index() == 0)
	{
		return nullptr;
	}

	return &std::get<2>(inputData.mValueToUseIfNotLinked);
}

const Engine::MetaAny* Engine::ScriptPin::TryGetValueIfNoInputLinked() const
{
	return const_cast<ScriptPin&>(*this).TryGetValueIfNoInputLinked();
}

void Engine::ScriptPin::SetValueIfNoInputLinked(MetaAny&& value)
{
	ASSERT(GetKind() == ScriptPinKind::Input);
	std::get<InputData>(mData).mValueToUseIfNotLinked = std::move(value);
}

void Engine::ScriptPin::SerializeTo(BinaryGSONObject& to) const
{
	to.AddGSONMember("id") << mId.Get();
	to.AddGSONMember("paramInfo") << mParamInfo;

	if (std::holds_alternative<OutputData>(mData))
	{
		to.AddGSONMember("isOutput") << true;
		return;
	}
	to.AddGSONMember("isOutput") << false;

	const InputData& inputData = std::get<InputData>(mData);

	to.AddGSONMember("inspectSize") << inputData.mInspectWindowSize;

	if (inputData.mValueToUseIfNotLinked.index() == 0)
	{
		return;
	}

	if (inputData.mValueToUseIfNotLinked.index() == 1)
	{
		to.AddGSONMember("defaultValueOfPin").SetData(std::get<1>(inputData.mValueToUseIfNotLinked));
		return;
	}

	const MetaAny* valueToSerialize = &std::get<2>(inputData.mValueToUseIfNotLinked);

	if (valueToSerialize == nullptr)
	{
		return;
	}

	const MetaType* const type = valueToSerialize->TryGetType();

	if (type == nullptr)
	{
		UNLIKELY;
		LOG(LogScripting, Warning, "Failed to serialize default value of pin: type {} was not reflected", 
			valueToSerialize->GetTypeId());
		return;
	}

	FuncResult defaultConstructedValue = type->Construct();

	auto serializeValue = 
		[&]
		{
			const FuncResult result = type->CallFunction(sSerializeMemberFuncName, to.AddGSONMember("defaultValueOfPin"), *valueToSerialize);

			if (result.HasError())
			{
				UNLIKELY;
				LOG(LogScripting, Warning, "Failed to serialize default value of pin: {}::{} returned {}",
					type->GetName(),
					sSerializeMemberFuncName.StringView(),
					result.Error());
				to.GetGSONMembers().pop_back();
			}
		};

	if (defaultConstructedValue.HasError())
	{
		serializeValue();
		return;
	}

	FuncResult equalResult = type->CallFunction(OperatorType::equal, defaultConstructedValue, *valueToSerialize);

	if (equalResult.HasError()
		|| !*equalResult.GetReturnValue().As<bool>())
	{
		serializeValue();
	}
}

std::optional<Engine::ScriptPin> Engine::ScriptPin::DeserializeFrom(const BinaryGSONObject& src, const ScriptNode& node, [[maybe_unused]] uint32 version)
{
	const BinaryGSONMember* serializedId = src.TryGetGSONMember("id");
	const BinaryGSONMember* serializedIsOutput = src.TryGetGSONMember("isOutput");

	if (serializedId == nullptr
		|| serializedIsOutput == nullptr)
	{
		UNLIKELY;
		LOG(LogScripting, Warning, "Failed to deserialize pin: missing values");
		return std::nullopt;
	}

	ScriptPin pin{ node.GetId() };

	PinId::ValueType tmp{};
	*serializedId >> tmp;
	pin.mId = tmp;

	const BinaryGSONMember* serializedParamInfo = src.TryGetGSONMember("paramInfo");

	if (serializedParamInfo == nullptr)
	{
		UNLIKELY;
			LOG(LogScripting, Warning, "Failed to deserialize pin: missing values");
		return std::nullopt;
	}

	*serializedParamInfo >> pin.mParamInfo;
	

	bool isOutputValue{};
	*serializedIsOutput >> isOutputValue;

	if (isOutputValue)
	{
		return pin;
	}

	pin.mData = InputData{};
	InputData& inputData = std::get<InputData>(pin.mData);

	const BinaryGSONMember* defaultValueOfPin = src.TryGetGSONMember("defaultValueOfPin");

	if (defaultValueOfPin != nullptr)
	{
		inputData.mValueToUseIfNotLinked = std::string{ defaultValueOfPin->GetData() };
	}

	const BinaryGSONMember* inspectSize = src.TryGetGSONMember("inspectSize");
	if (inspectSize != nullptr)
	{
		*inspectSize >> inputData.mInspectWindowSize;
	}

	return pin;
}

void Engine::ScriptPin::CollectErrors(ScriptErrorInserter inserter, const ScriptFunc& scriptFunc) const
{
	if (IsFlow())
	{
		return;
	}

	const MetaType* const type = TryGetType();

	if (type == nullptr)
	{
		inserter = { ScriptError::Type::UnreflectedType, { scriptFunc, *this }, GetTypeName() };
		return;
	}


	if (!CanTypeBeUsedInScripts(*type, GetTypeForm()))
	{
		inserter = { ScriptError::Type::TypeCannotBeReferencedFromScripts, { scriptFunc, *this }, GetTypeName() };
	}
}

void Engine::ScriptPin::PostDeclarationRefresh()
{
	mParamInfo.RefreshTypePointer();

	if (GetKind() == ScriptPinKind::Output)
	{
		return;
	}

	auto& valueToUseIfNotLinked = std::get<InputData>(mData).mValueToUseIfNotLinked;

	if (valueToUseIfNotLinked.index() == 0
		|| valueToUseIfNotLinked.index() == 2)
	{
		return;
	}

	const MetaType* type = TryGetType();

	if (type == nullptr)
	{
		LOG(LogScripting, Error, "Failed to deserialize pin default value: type {} no longer exists", GetTypeName());
		valueToUseIfNotLinked = std::monostate{};
		return;
	}

	FuncResult defaultConstructedResult = type->Construct();

	if (defaultConstructedResult.HasError())
	{
		LOG(LogScripting, Warning, "Failed to deserialize default value of pin, type {} could not be default constructed - {}",
			type->GetName(),
			defaultConstructedResult.Error());
		valueToUseIfNotLinked = std::monostate{};
		return;
	}

	std::string serializedValue = std::move(std::get<1>(valueToUseIfNotLinked));
	valueToUseIfNotLinked = std::move(defaultConstructedResult.GetReturnValue());

	const MetaFunc* const deserializeFunc = type->TryGetFunc(sDeserializeMemberFuncName);

	if (deserializeFunc == nullptr)
	{
		LOG(LogScripting, Warning, "Failed to deserialize pin default value: type {} has no {} function",
			type->GetName(),
			sDeserializeMemberFuncName.StringView());
		return;
	}

	BinaryGSONMember serializedGSONMember{};
	serializedGSONMember.SetData(std::move(serializedValue));

	const FuncResult result = (*deserializeFunc)(serializedGSONMember, std::get<2>(valueToUseIfNotLinked));

	if (result.HasError())
	{
		UNLIKELY;
		LOG(LogScripting, Warning, "Failed to deserialize default value of pin: {}::{} returned {}",
			type->GetName(),
			sDeserializeMemberFuncName.StringView(),
			result.Error());
	}
}
