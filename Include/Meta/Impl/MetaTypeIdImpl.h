#pragma once
#include "Meta/Fwd/MetaTypeIdFwd.h"

template<typename T>
CONSTEVAL CE::TypeId CE::MakeTypeId()
{
	return entt::type_hash<T>::value();
}

template<typename T>
CONSTEVAL std::string_view CE::MakeTypeName()
{
	return entt::type_name<T>::value();
}