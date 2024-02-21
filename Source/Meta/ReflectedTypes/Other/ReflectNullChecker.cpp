#include "Precomp.h"
#include "Meta/MetaReflect.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

using namespace Engine;

struct Null
{
	static bool IsNull(const MetaAny* any);
	static bool IsNotNull(const MetaAny* any);

	static MetaType Reflect()
	{
		MetaType type{ MetaType::T<Null>{}, "NullChecker" };

		type.GetProperties().Add(Props::sIsScriptableTag);

		type.AddFunc(&IsNull, "IsNull").GetProperties().Add(Props::sIsScriptableTag);
		type.AddFunc(&IsNotNull, "IsNotNull").GetProperties().Add(Props::sIsScriptableTag);

		return type;
	}

	static bool CallNullptrOverload(const MetaAny& any);
};

bool Null::IsNull(const MetaAny* any)
{
	if (any == nullptr
		|| *any == nullptr)
	{
		return true;
	}

	return CallNullptrOverload(*any);
}

bool Null::IsNotNull(const MetaAny* any)
{
	if (any == nullptr
		|| *any == nullptr)
	{
		return false;
	}

	return !CallNullptrOverload(*any);
}

bool Null::CallNullptrOverload(const MetaAny& any)
{
	static constexpr TypeTraits nullptrTraits = MakeTypeTraits<nullptr_t>();

	const MetaType* type = any.TryGetType();

	if (type == nullptr)
	{
		return true;
	}

	const MetaFunc* const nullptrComparison = type->TryGetFunc(OperatorType::equal, MakeFuncId(MakeTypeTraits<bool>(),
		{ TypeTraits{ type->GetTypeId(), TypeForm::ConstRef }, nullptrTraits }));

	if (nullptrComparison == nullptr)
	{
		return true;
	}

	nullptr_t null{};
	const MetaAny nullRef{ null };

	bool retAddress{};
	FuncResult nullptrComparisonResult = (*nullptrComparison).InvokeUncheckedUnpackedWithRVO(&retAddress, any, nullRef);

	if (nullptrComparisonResult.HasError())
	{
		LOG(LogScripting, Error, "An error occured when invoking the IsNull overload for type {} - {}",
			type->GetName(),
			nullptrComparisonResult.Error())
			return true;
	}

	ASSERT(nullptrComparisonResult.HasReturnValue()
		&& nullptrComparisonResult.GetReturnValue().GetTypeId() == MakeTypeId<bool>()
		&& "IsNull overload unexpectedly returned nullptr or a non - boolean, should not be possible")

		ASSERT(nullptrComparisonResult.GetReturnValue().GetData() == &retAddress);
	return retAddress;
}

REFLECT_AT_START_UP(Null);

