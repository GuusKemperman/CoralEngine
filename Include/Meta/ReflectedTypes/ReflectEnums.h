#pragma once
#include "magic_enum/magic_enum.hpp"
#include "Utilities/Reflect/ReflectFieldType.h"

namespace CE
{
	template<typename T>
	MetaType ReflectEnumType(bool isScriptable) requires magic_enum::detail::is_enum_v<T>
	{
		MetaType type{ MetaType::T<T>{}, magic_enum::enum_type_name<T>() };

		if (isScriptable)
		{
			type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

			type.AddFunc(std::equal_to<T>(), OperatorType::equal).GetProperties().Add(Props::sIsScriptableTag);
			type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal).GetProperties().Add(Props::sIsScriptableTag);
		}

		ReflectFieldType<T>(type);
		return type;
	}
}
