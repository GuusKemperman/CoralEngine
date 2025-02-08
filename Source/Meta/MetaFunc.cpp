#include "Precomp.h"
#include "Meta/MetaFunc.h"

#include "Meta/MetaType.h"
#include "Meta/MetaFuncId.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaProps.h"

CE::MetaFunc::MetaFunc(const InvokeT& funcToInvoke,
	const NameOrTypeInit nameOrType,
	const Return& returnType,
	const Params& parameters) :
	mReturn(returnType),
	mParams(parameters),
	mNameOrType(std::holds_alternative<std::string_view>(nameOrType) ?
		NameOrType{ std::string{ std::get<std::string_view>(nameOrType) } } : NameOrType{ std::get<OperatorType>(nameOrType) }),
	mProperties(std::make_unique<MetaProps>()),
	mFuncId(MakeFuncId(mReturn.mTypeTraits,
		[&]
		{
			std::vector<TypeTraits> params{};
			params.reserve(mParams.size());

			for (const MetaFuncNamedParam& namedParam : mParams)
			{
				params.emplace_back(namedParam.mTypeTraits);
			}

			return params;
		}())),
	mFuncToInvoke(funcToInvoke)
{
}

CE::MetaFunc::MetaFunc(InvokeT&& func, NameOrType&& nameOrOp, std::initializer_list<TypeTraits> paramTypes,
	std::initializer_list<std::string_view> paramNames, FuncId funcId, ExplicitlyUseThisConstructor) :
	mReturn({ *(paramTypes.end() - 1), paramNames.size() >= paramTypes.size() ? *(paramNames.end() - 1) : std::string_view{}}),
	mParams([&paramNames, &paramTypes]()
		{
			std::vector<MetaFuncNamedParam> params{};
			const auto typesStart = paramTypes.begin();
			const auto typesEnd = paramTypes.end() - 1;

			const auto namesStart = paramNames.begin();
			const auto namesEnd = namesStart + std::min(paramNames.size(), paramTypes.size() - 1);

			params.reserve(std::distance(typesStart, typesEnd));

			auto typeIt = typesStart;
			auto nameIt = namesStart;

			for (; typeIt < typesEnd; ++typeIt, ++nameIt)
			{
				params.emplace_back(*typeIt, paramNames.size() > 0 && nameIt < namesEnd ? *nameIt : std::string_view{});
			}

			return params;
		}()),
	mNameOrType(std::move(nameOrOp)),
	mProperties(std::make_unique<MetaProps>()),
	mFuncId(funcId),
	mFuncToInvoke(std::move(func))
{
}

CE::MetaFunc::MetaFunc(InvokeT&& func, OperatorType op, std::initializer_list<TypeTraits> paramTypes,
	std::initializer_list<std::string_view> paramNames, FuncId funcId) :
	MetaFunc(std::move(func), NameOrType{ op }, paramTypes, paramNames, funcId, ExplicitlyUseThisConstructor{})
{
}

CE::MetaFunc::MetaFunc(InvokeT&& func, std::string_view name, std::initializer_list<TypeTraits> paramTypes,
	std::initializer_list<std::string_view> paramNames, FuncId funcId) :
	MetaFunc(std::move(func), NameOrType{ std::string{ name } }, paramTypes, paramNames, funcId, ExplicitlyUseThisConstructor{})
{
}

CE::MetaFunc::MetaFunc(MetaFunc&&) noexcept = default;

CE::MetaFunc::~MetaFunc() = default;

CE::MetaFunc& CE::MetaFunc::operator=(MetaFunc&& other) noexcept = default;

std::string_view CE::MetaFunc::GetDesignerFriendlyName() const
{
	return GetDesignerFriendlyName(mNameOrType);
}

std::string_view CE::MetaFunc::GetDesignerFriendlyName(const NameOrType& nameOrType)
{
	return std::holds_alternative<OperatorType>(nameOrType) ?
		GetNameOfOperator(std::get<OperatorType>(nameOrType)) :
		std::get<std::string>(nameOrType);
}

CE::FuncResult CE::MetaFunc::InvokeChecked(std::span<MetaAny> args, std::span<const TypeForm> formOfArgs, RVOBuffer rvoBuffer) const
{
	std::optional<std::string> reasonWeCantInvoke = CanArgBePassedIntoParam(args, formOfArgs, mParams);

	if (reasonWeCantInvoke.has_value())
	{
		return std::move(*reasonWeCantInvoke);
	}

	return InvokeUnchecked(args, formOfArgs, rvoBuffer);
}

CE::FuncResult CE::MetaFunc::InvokeUnchecked(std::span<MetaAny> args,
	[[maybe_unused]] std::span<const TypeForm> formOfArgs, RVOBuffer rvoBuffer) const
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

std::optional<std::string> CE::MetaFunc::CanArgBePassedIntoParam(std::span<const MetaAny> args,
	std::span<const TypeForm> formOfArgs,
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

std::optional<std::string> CE::MetaFunc::CanArgBePassedIntoParam(const MetaAny& arg, TypeForm formOfArg, const TypeTraits param)
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

std::optional<std::string> CE::MetaFunc::CanArgBePassedIntoParam(TypeTraits arg, TypeTraits param)
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
CE::MetaAny CE::MetaFunc::PackSingle<const CE::MetaAny&>(const MetaAny& other)
{
	return { other.GetTypeInfo(), const_cast<void*>(other.GetData())};
}

template <>
CE::MetaAny CE::MetaFunc::PackSingle<CE::MetaAny&>(MetaAny& other)
{
	return { other.GetTypeInfo(), other.GetData() };
}

CE::MetaAny& CE::FuncResult::GetReturnValue()
{
	ASSERT(HasReturnValue());
	return *std::get<std::optional<MetaAny>>(mResult);
}

bool CE::FuncResult::HasReturnValue() const
{
	return std::holds_alternative<std::optional<MetaAny>>(mResult)
		&& std::get<std::optional<MetaAny>>(mResult).has_value();
}

bool CE::FuncResult::HasError() const
{
	return std::holds_alternative<std::string>(mResult);
}

std::string CE::FuncResult::Error() const
{
	if (HasError())
	{
		return std::get<std::string>(mResult);
	}
	LOG(LogMeta, Warning, "Attempted to get error value out of successful function call");
	return std::string();
}

std::string_view CE::GetNameOfOperator(const OperatorType type)
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
	case OperatorType::increment: return "++";
	case OperatorType::decrement: return "--";
	default: return "Invalid operator";
	}
}
