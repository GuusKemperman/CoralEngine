#include "Precomp.h"
#include "Meta/MetaReflect.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

using namespace CE;

namespace
{
	bool IsNull(const MetaAny* any);
	bool IsNotNull(const MetaAny* any);
}

MetaType Reflector<Internal::Null>::Reflect()
{
	MetaType type{ MetaType::T<Internal::Null>{}, "Null" };

	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&IsNull, "IsNull").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&IsNotNull, "IsNotNull").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}

namespace
{
	bool IsNull(const MetaAny* any)
	{
		if (any == nullptr
			|| *any == nullptr)
		{
			return true;
		}

		static constexpr TypeTraits nullptrTraits = MakeTypeTraits<nullptr_t>();

		const MetaType* type = any->TryGetType();

		if (type == nullptr)
		{
			return false;
		}

		const MetaFunc* const nullptrComparison = type->TryGetFunc(OperatorType::equal, MakeFuncId(MakeTypeTraits<bool>(),
			{ TypeTraits{ type->GetTypeId(), TypeForm::ConstRef }, nullptrTraits }));

		if (nullptrComparison == nullptr)
		{
			return false;
		}

		nullptr_t null{};
		const MetaAny nullRef{ null };

		bool retAddress{};
		FuncResult nullptrComparisonResult = nullptrComparison->InvokeUncheckedUnpackedWithRVO(&retAddress, *any, nullRef);

		if (nullptrComparisonResult.HasError())
		{
			LOG(LogScripting, Error, "An error occured when invoking the IsNull overload for type {} - {}",
				type->GetName(),
				nullptrComparisonResult.Error());
			return false;
		}

		ASSERT(nullptrComparisonResult.HasReturnValue()
			&& nullptrComparisonResult.GetReturnValue().GetTypeId() == MakeTypeId<bool>()
			&& "IsNull overload unexpectedly returned nullptr or a non - boolean, should not be possible");

		ASSERT(nullptrComparisonResult.GetReturnValue().GetData() == &retAddress);
		return retAddress;
	}

	bool IsNotNull(const MetaAny* any)
	{
		return !IsNull(any);
	}
}