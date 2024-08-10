#pragma once

namespace CE
{
	using TypeId = uint32;

	template<typename T>
	constexpr TypeId MakeTypeId();

	template<typename T>
	constexpr std::string_view MakeTypeName();
}