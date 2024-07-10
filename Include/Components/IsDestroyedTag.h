#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class IsDestroyedTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(IsDestroyedTag);
	};
}