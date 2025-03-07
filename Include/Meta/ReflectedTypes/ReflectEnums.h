#pragma once
#include "Utilities/Reflect/ReflectFieldType.h"

namespace CE
{
	template<typename T>
	void ReflectEnumType(MetaType& type)
	{
		ASSERT(MakeTypeId<T>() == type.GetTypeId());

		if (type.GetProperties().Has(Props::sIsScriptableTag))
		{
			type.GetProperties().Add(Props::sIsScriptOwnableTag);
		}

		ReflectFieldType<T>(type);
	}
}
