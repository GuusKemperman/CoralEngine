#pragma once

namespace CE
{
	using TypeId = uint32;

	template<typename T>
	CONSTEVAL TypeId MakeTypeId();

	template<typename T>
	CONSTEVAL std::string_view MakeTypeName();
}