#pragma once
#include "Meta/Fwd/MetaTypeIdFwd.h"

template<typename T>
constexpr CE::TypeId CE::MakeTypeId()
{
	return entt::type_hash<T>::value();
}

template<typename T>
constexpr std::string_view CE::MakeTypeName()
{
	return entt::type_name<T>::value();
}