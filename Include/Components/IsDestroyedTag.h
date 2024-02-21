#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class IsDestroyedTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(IsDestroyedTag);
	};


}
