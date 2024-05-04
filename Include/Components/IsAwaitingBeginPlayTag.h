#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class IsAwaitingBeginPlayTag
	{
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(IsAwaitingBeginPlayTag);
	};
}
