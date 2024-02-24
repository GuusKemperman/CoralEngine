#pragma once
#include "Meta/Fwd/MetaTypeIdFwd.h"

template<typename T>
CONSTEVAL Engine::TypeId Engine::MakeTypeId()
{
	return entt::type_hash<T>::value();
}

template<typename T>
CONSTEVAL std::string_view Engine::MakeTypeName()
{
	return entt::type_name<T>::value();
}