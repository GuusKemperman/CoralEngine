#include "Precomp.h"
#include "Scripting/ScriptField.h"

#include "Assets/Script.h"
#include "GSON/GSONBinary.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Meta/MetaManager.h"
#include "Scripting/ScriptTools.h"

Engine::ScriptField::ScriptField(const Script& script, const std::string_view name) :
	mTypeData(MakeTypeTraits<int32>(), name),
	mNameOfScriptAsset(script.GetName()),
	mDefaultValue(std::make_unique<MetaAny>( int32{} ))
{
}

Engine::ScriptField::ScriptField(ScriptField&& other) noexcept :
	mTypeData(std::move(other.mTypeData)),
	mNameOfScriptAsset(std::move(other.mNameOfScriptAsset)),
	mDefaultValue(std::move(other.mDefaultValue))
{
}

Engine::ScriptField& Engine::ScriptField::operator=(ScriptField&& other) noexcept
{
	mTypeData = std::move(other.mTypeData);
	mNameOfScriptAsset = std::move(other.mNameOfScriptAsset);
	mDefaultValue = std::move(other.mDefaultValue);
	return *this;
}

Engine::ScriptField::~ScriptField() = default;

void Engine::ScriptField::SetType(const MetaType& type)
{
	if (!CanTypeBeUsedForFields(type))
	{
		LOG(LogScripting, Error, "Cannot set type of {}::{} to {} - This type cannot be used for datamembers. First check using CanTypeBeUsedForFields",
			mNameOfScriptAsset,
			GetName(),
			type.GetName());
		return;
	}

	mTypeData = ScriptVariableTypeData{  type, TypeForm::Value, std::string{ GetName() }, };
	mDefaultValue = std::make_unique<MetaAny>(std::move(type.Construct().GetReturnValue()));
}

bool Engine::ScriptField::CanTypeBeUsedForFields(const MetaType& type)
{
	return CanTypeBeOwnedByScripts(type)
		&& type.TryGetFunc(OperatorType::equal, MakeFuncId(MakeTypeTraits<bool>(), { { type.GetTypeId(), TypeForm::ConstRef }, { type.GetTypeId(), TypeForm::ConstRef } })) != nullptr
		&& type.TryGetFunc(sSerializeMemberFuncName) != nullptr
		&& type.TryGetFunc(sDeserializeMemberFuncName) != nullptr;
}

void Engine::ScriptField::SerializeTo(BinaryGSONObject& object) const
{
	object.AddGSONMember("typeData") << mTypeData;

	const MetaType* const type = TryGetType();

	if (type == nullptr)
	{
		LOG_TRIVIAL(LogScripting, Warning, "Metafield type was nullptr, default value will not be serialized.");
		return;
	}

	if (WasValueDefaultConstructed())
	{
		return;
	}

	const MetaAny& defaultValue = GetDefaultValue();

	[[maybe_unused]] const FuncResult serializeResult = type->CallFunction(sSerializeMemberFuncName, object.AddGSONMember("defaultValue"), defaultValue);
	ASSERT_LOG(!serializeResult.HasError(), "{}", serializeResult.Error());
}

std::optional<Engine::ScriptField> Engine::ScriptField::DeserializeFrom(const BinaryGSONObject& object, const Script& owningScript, [[maybe_unused]] const uint32 version)
{
	const BinaryGSONMember* const serializedTypeData = object.TryGetGSONMember("typeData");

	if (serializedTypeData == nullptr)
	{
		UNLIKELY;
		LOG_TRIVIAL(LogScripting, Warning, "Failed to deserialize script data field, missing values");
		return std::nullopt;
	}

	ScriptVariableTypeData typeData{};

	*serializedTypeData >> typeData;

	ScriptField field{ owningScript, typeData.GetName() };

	const MetaType* const type = typeData.TryGetType();

	if (type == nullptr)
	{
		LOG(LogScripting, Warning, "Script {} has field {}, but it's type {} no longer exists. Switching to a different type", owningScript.GetName(), typeData.GetName(), typeData.GetTypeName());
		field.SetType(MetaManager::Get().GetType<int32>());
		return std::move(field);
	}

	if (!CanTypeBeUsedForFields(*type))
	{
		LOG(LogScripting, Warning, "Script {} has field {} and was previously of type {}, but this type can no longer be used as a field. Switching to a different type",
			owningScript.GetName(),
			typeData.GetName(),
			type->GetName());
		field.SetType(MetaManager::Get().GetType<int32>());
		return std::move(field);
	}

	field.SetType(*type);
	const BinaryGSONMember* const serializedDefaultValue = object.TryGetGSONMember("defaultValue");
	field.mDefaultValue = serializedDefaultValue == nullptr ? std::string{} : std::string{ serializedDefaultValue->GetData() };

	return std::move(field);
}

void Engine::ScriptField::CollectErrors(ScriptErrorInserter inserter, const Script& owningScript) const
{
	ASSERT(owningScript.GetName() == mNameOfScriptAsset);

	const ptrdiff_t numberWithThisName = std::count_if(owningScript.GetFields().begin(), owningScript.GetFields().end(),
		[name = GetName()](const ScriptField& field)
		{
			return field.GetName() == name;
		});

	if (numberWithThisName != 1)
	{
		if (numberWithThisName == 0)
		{
			LOG(LogScripting, Error, "OwningScript {} was not the script that owns {}, found while collecting errors",
				owningScript.GetName(),
				GetName());
		}

		inserter = { ScriptError::Type::NameNotUnique, *this };
	}

	const MetaType* const type = TryGetType();

	if (type == nullptr)
	{
		inserter = { ScriptError::Type::UnreflectedType, *this };
		return;
	}

	if (!CanTypeBeUsedForFields(*type))
	{
		inserter = { ScriptError::Type::TypeCannotBeMember, *this };
	}
}

Engine::MetaAny& Engine::ScriptField::GetDefaultValue()
{
	if (mDefaultValue.index() == 0)
	{
		const MetaType* const type = TryGetType();

		// Is already checked during loading
		ASSERT_LOG(type != nullptr, "Type of field {} on script {} no longer exists", GetName(), GetNameOfScriptAsset());
		ASSERT_LOG(CanTypeBeUsedForFields(*type), "Type of field {} on script {} cannot be used for scripts", GetName(), GetNameOfScriptAsset());

		std::string defaultValueAsString = std::move(std::get<0>(mDefaultValue));

		mDefaultValue = std::make_unique<MetaAny>(std::move(type->Construct().GetReturnValue()));

		if (!defaultValueAsString.empty())
		{
			BinaryGSONMember serializedDefaultValue{};
			serializedDefaultValue.SetData(std::move(defaultValueAsString));

			const FuncResult deserializeResult = type->CallFunction(sDeserializeMemberFuncName, serializedDefaultValue, *std::get<1>(mDefaultValue));
			ASSERT_LOG(!deserializeResult.HasError(), "{}", deserializeResult.Error());
		}
		}

	return *std::get<1>(mDefaultValue);
}

const Engine::MetaAny& Engine::ScriptField::GetDefaultValue() const
{
	return const_cast<ScriptField&>(*this).GetDefaultValue();
}

bool Engine::ScriptField::WasValueDefaultConstructed() const
{
	if (mDefaultValue.index() == 0)
	{
		return std::get<0>(mDefaultValue).empty();
	}

	const MetaType& type = *TryGetType();

	const MetaAny& defaultValue = GetDefaultValue();
	MetaAny defaultConstructedValue = std::move(type.Construct().GetReturnValue());
	ASSERT(defaultConstructedValue != nullptr);

	[[maybe_unused]] FuncResult equalResult = type.CallFunction(OperatorType::equal, defaultConstructedValue, defaultValue);
	ASSERT_LOG(!equalResult.HasError(), "{}", equalResult.Error());

	return *equalResult.GetReturnValue().As<bool>();
}
