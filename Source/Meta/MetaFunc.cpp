#include "Precomp.h"
#include "Meta/MetaFunc.h"

#include "Meta/MetaType.h"
#include "Meta/MetaFuncId.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"

Engine::MetaFunc::MetaFunc(const InvokeT& funcToInvoke,
	const NameOrTypeInit nameOrType,
	const Return& returnType,
	const Params& parameters) :
	mReturn(returnType),
	mParams(parameters),
	mNameOrType(std::holds_alternative<std::string_view>(nameOrType) ?
		NameOrType{ std::string{ std::get<std::string_view>(nameOrType) } } : NameOrType{ std::get<OperatorType>(nameOrType) }),
	mProperties(std::make_unique<MetaProps>()),
	mFuncId(MakeFuncId(mReturn.mTypeTraits, std::vector<TypeTraits>{ mParams.data(), mParams.data() + mParams.size() })),
	mFuncToInvoke(funcToInvoke)
{
}

Engine::MetaFunc::MetaFunc(InvokeT&& func,
	const NameOrTypeInit nameOrType, Params&& paramsAndReturnAtBack, uint32 funcId) :
	mReturn(std::move(paramsAndReturnAtBack.back())),
	mParams(std::move(paramsAndReturnAtBack)),
	mNameOrType(std::holds_alternative<std::string_view>(nameOrType) ?
		NameOrType{ std::string{ std::get<std::string_view>(nameOrType) } } : NameOrType{ std::get<OperatorType>(nameOrType) }),
	mProperties(std::make_unique<MetaProps>()),
	mFuncId(funcId),
	mFuncToInvoke(std::move(func))
{
	mParams.pop_back();
}

Engine::MetaFunc::MetaFunc(MetaFunc&&) noexcept = default;

Engine::MetaFunc::~MetaFunc() = default;

Engine::MetaFunc& Engine::MetaFunc::operator=(MetaFunc&& other) noexcept = default;

std::string_view Engine::MetaFunc::GetDesignerFriendlyName() const
{
	return GetDesignerFriendlyName(mNameOrType);
}

std::string_view Engine::MetaFunc::GetDesignerFriendlyName(const NameOrType& nameOrType)
{
	return std::holds_alternative<OperatorType>(nameOrType) ?
		GetNameOfOperator(std::get<OperatorType>(nameOrType)) :
		std::get<std::string>(nameOrType);
}

Engine::FuncResult Engine::MetaFunc::InvokeChecked(Span<MetaAny> args, Span<const TypeForm> formOfArgs, RVOBuffer rvoBuffer) const
{
	std::optional<std::string> reasonWeCantInvoke = CanArgBePassedIntoParam(args, formOfArgs, mParams);

	if (reasonWeCantInvoke.has_value())
	{
		return std::move(*reasonWeCantInvoke);
	}

	return InvokeUnchecked(args, formOfArgs, rvoBuffer);
}

Engine::FuncResult Engine::MetaFunc::InvokeUnchecked(Span<MetaAny> args,
	[[maybe_unused]] Span<const TypeForm> formOfArgs, RVOBuffer rvoBuffer) const
{
	ASSERT_LOG(!CanArgBePassedIntoParam(args, formOfArgs, mParams).has_value(), "Invalid arguments passed to function - {} - Use invoke checked if you are not sure your arguments are valid",
		*CanArgBePassedIntoParam(args, formOfArgs, mParams));

	if (!mFuncToInvoke)
	{
		UNLIKELY;
		return FuncResult{ "No invoke function! (Should never happen)" };
	}

	return mFuncToInvoke(args, rvoBuffer);
}

std::optional<std::string> Engine::MetaFunc::CanArgBePassedIntoParam(Span<const MetaAny> args,
	Span<const TypeForm> formOfArgs,
	const std::vector<MetaFuncNamedParam>& params)
{
	if (args.size() != formOfArgs.size())
	{
		LOG(LogMeta, Error, "Num of args ({}) did not match num of forms provided ({})", args.size(), formOfArgs.size());
		return Format("Num of args ({}) did not match num of forms provided ({})", args.size(), formOfArgs.size());
	}

	if (args.size() != params.size())
	{
		return Format("Expected {} arguments, but received {}", params.size(), args.size());
	}

	for (uint32 i = 0; i < args.size(); i++)
	{
		std::optional<std::string> reasonWhyWeCannotPassItIn = CanArgBePassedIntoParam(args[i], formOfArgs[i], params[i].mTypeTraits);

		if (reasonWhyWeCannotPassItIn.has_value())
		{
			return Format("Cannot pass argument {} into param - {}", params[i].mName, std::move(*reasonWhyWeCannotPassItIn));
		}
	}

	return std::nullopt;
}

std::optional<std::string> Engine::MetaFunc::CanArgBePassedIntoParam(const MetaAny& arg, TypeForm formOfArg, const TypeTraits param)
{
	std::optional<std::string> reasonWhyWeCannotPassItIn = CanArgBePassedIntoParam({ arg.GetTypeId(), formOfArg }, param);

	if (reasonWhyWeCannotPassItIn.has_value())
	{
		return reasonWhyWeCannotPassItIn;
	}

	if (arg == nullptr
		&& !CanFormBeNullable(param.mForm))
	{
		return "Arg was nullptr";
	}

	return std::nullopt;
}

std::optional<std::string> Engine::MetaFunc::CanArgBePassedIntoParam(TypeTraits arg, TypeTraits param)
{
	static constexpr std::string_view constPassedToMutable = "Cannot pass const value to mutable parameter";
	static constexpr std::string_view argNotMovedIn = "Expected an RValue, but the argument was not moved in";
	static constexpr std::string_view expectedLValue = "The argument was not an L-value";

	std::string_view errorMessage{};

	switch (arg.mForm)
	{
	case TypeForm::RValue:
	case TypeForm::Value:
	{
		switch (param.mForm)
		{
		case TypeForm::Ref:
		case TypeForm::Ptr:
		{
			errorMessage = expectedLValue;
			break;
		}
		case TypeForm::ConstRef:
		case TypeForm::ConstPtr:
		case TypeForm::RValue:
		case TypeForm::Value:;
		}
		break;
	}
	case TypeForm::Ref:
	{
		switch (param.mForm)
		{
		case TypeForm::RValue:
		{
			errorMessage = argNotMovedIn;
			break;
		}
		case TypeForm::Value:
		case TypeForm::Ref:
		case TypeForm::ConstRef:
		case TypeForm::Ptr:
		case TypeForm::ConstPtr:;
		}
		break;
	}
	case TypeForm::ConstRef:
	{
		switch (param.mForm)
		{
		case TypeForm::Ref:
		case TypeForm::Ptr:
		{
			errorMessage = constPassedToMutable;
			break;
		}
		case TypeForm::RValue:
		{
			errorMessage = argNotMovedIn;
			break;
		}
		case TypeForm::Value:
		case TypeForm::ConstRef:
		case TypeForm::ConstPtr:;
		}
		break;
	}
	case TypeForm::Ptr:
	{
		switch (param.mForm)
		{
		case TypeForm::RValue:
		{
			errorMessage = argNotMovedIn;
			break;
		}
		case TypeForm::Value:
		case TypeForm::ConstRef:
		case TypeForm::ConstPtr:
		case TypeForm::Ref:
		case TypeForm::Ptr:;
		}
		break;
	}
	case TypeForm::ConstPtr:
	{
		switch (param.mForm)
		{
		case TypeForm::Ref:
		case TypeForm::Ptr:
		{
			errorMessage = constPassedToMutable;
			break;
		}
		case TypeForm::RValue:
		{
			errorMessage = argNotMovedIn;
			break;
		}
		case TypeForm::Value:
		case TypeForm::ConstRef:
		case TypeForm::ConstPtr:;
		}
		break;
	}
	}

	if (!errorMessage.empty())
	{
		return Format("Could not pass argument of form {} to parameter of form {} - {}",
			EnumToString(arg.mForm),
			EnumToString(param.mForm),
			errorMessage);
	}

	if (arg.mStrippedTypeId == param.mStrippedTypeId)
	{
		return std::nullopt;
	}

	if (param.mStrippedTypeId == MakeTypeId<MetaAny>())
	{
		return std::nullopt;
	}

	const MetaType* const receivedType = MetaManager::Get().TryGetType(arg.mStrippedTypeId);

	if (receivedType != nullptr)
	{
		if (receivedType->IsDerivedFrom(param.mStrippedTypeId))
		{
			return std::nullopt;
		}

		return Format("{} does not derive from the parameter type", receivedType->GetName());
	}

	const MetaType* const expectedType = MetaManager::Get().TryGetType(param.mStrippedTypeId);

	if (expectedType == nullptr)
	{
		return Format("Both the argument and the parameter were of unreflected types, and their typeIds\
 were different; typeId of arg was {} while typeId of param was {}", arg.mStrippedTypeId, param.mStrippedTypeId);
	}

	if (!expectedType->IsBaseClassOf(arg.mStrippedTypeId))
	{
		return Format("arg was of an unreflected type, and does not derive from {}", expectedType->GetName());
	}

	return std::nullopt;
}

template <>
Engine::MetaAny Engine::MetaFunc::PackSingle<const Engine::MetaAny&>(const MetaAny& other)
{
	return { other.GetTypeInfo(), const_cast<void*>(other.GetData())};
}

template <>
Engine::MetaAny Engine::MetaFunc::PackSingle<Engine::MetaAny&>(MetaAny& other)
{
	return { other.GetTypeInfo(), other.GetData() };
}

Engine::MetaAny& Engine::FuncResult::GetReturnValue()
{
	ASSERT(HasReturnValue());
	return *std::get<std::optional<MetaAny>>(mResult);
}

bool Engine::FuncResult::HasReturnValue() const
{
	return std::holds_alternative<std::optional<MetaAny>>(mResult)
		&& std::get<std::optional<MetaAny>>(mResult).has_value();
}

bool Engine::FuncResult::HasError() const
{
	return std::holds_alternative<std::string>(mResult);
}

std::string Engine::FuncResult::Error() const
{
	if (HasError())
	{
		return std::get<std::string>(mResult);
	}
	LOG_TRIVIAL(LogMeta, Warning, "Attempted to get error value out of successful function call");
	return std::string();
}

std::string_view Engine::GetNameOfOperator(const OperatorType type)
{
	switch (type)
	{
	case OperatorType::constructor: return  "Constructor";
	case OperatorType::destructor: return  "Destructor";
	case OperatorType::assign: return "=";
	case OperatorType::negate: return "!";
	case OperatorType::complement: return "~";
	case OperatorType::indirect: return "*";
	case OperatorType::address_of: return "&";
	case OperatorType::add: return "+";
	case OperatorType::subtract: return "-";
	case OperatorType::multiplies: return "*";
	case OperatorType::divides: return "/";
	case OperatorType::modulus: return "%";
	case OperatorType::equal: return "==";
	case OperatorType::inequal: return "!=";
	case OperatorType::greater: return ">";
	case OperatorType::less: return "<";
	case OperatorType::greater_equal: return ">=";
	case OperatorType::less_equal: return "<=";
	case OperatorType::logical_and: return "&&";
	case OperatorType::logical_or: return "||";
	case OperatorType::bitwise_and: return "&";
	case OperatorType::bitwise_or: return "|";
	case OperatorType::bitwise_xor: return "^";
	case OperatorType::left_shift: return "<<";
	case OperatorType::right_shift: return ">>";
	case OperatorType::add_assign: return "+=";
	case OperatorType::subtract_assign: return "-=";
	case OperatorType::multiplies_assign: return "*=";
	case OperatorType::divides_assign: return "/=";
	case OperatorType::modulus_assign: return "%=";
	case OperatorType::right_shift_assign: return ">>";
	case OperatorType::left_shift_assign: return "<<";
	case OperatorType::bitwise_and_assign: return "&=";
	case OperatorType::bitwise_xor_assign: return "^=";
	case OperatorType::bitwise_or_assign: return "|=";
	case OperatorType::arrow_indirect: return "->";
	case OperatorType::comma: return ",";
	case OperatorType::subscript: return "[]";
	case OperatorType::arrow: return "->";
	case OperatorType::dot: return ".";
	case OperatorType::dot_indirect: return ".*";
	default: return "Invalid operator";
	}
}
