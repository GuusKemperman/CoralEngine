#pragma once
#include "Utilities/MemFunctions.h"

namespace Engine
{
	class MetaAny;

	MetaAny MakeRef(MetaAny& anotherAny);

	template<typename T>
	std::shared_ptr<T> MakeShared(MetaAny&& any);

	template<typename T>
	std::unique_ptr<T, InPlaceDeleter<T, true>> MakeUnique(MetaAny&& any);
}
