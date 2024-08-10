#pragma once
#include "MetaTypeTraitsFwd.h"

namespace CE
{
	using FuncId = uint32;

	FuncId MakeFuncId(TypeTraits returnType, const std::vector<TypeTraits>& parameters);

	template<typename T>
	constexpr FuncId MakeFuncId();
};