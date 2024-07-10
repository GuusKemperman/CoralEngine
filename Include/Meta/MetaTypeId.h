#pragma once

namespace Engine
{
	class MetaType;

	using TypeId = uint32;

	template<typename T>
	static CONSTEVAL uint32 MakeTypeId() 
	{ 
		return entt::type_hash<T>::value(); 
	}

	template<typename T>
	static CONSTEVAL std::string_view MakeTypeName()
	{
		return entt::type_name<T>::value();
	}
}