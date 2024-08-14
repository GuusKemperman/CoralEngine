#pragma once

namespace CE
{
	using TypeId = uint32;

	template<typename T>
	consteval TypeId MakeTypeId();

	template<typename T>
	consteval std::string_view MakeTypeName();
}