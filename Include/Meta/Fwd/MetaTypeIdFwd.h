#pragma once

namespace Engine
{
	using TypeId = uint32;

	template<typename T>
	CONSTEVAL TypeId MakeTypeId();

	template<typename T>
	CONSTEVAL std::string_view MakeTypeName();
}