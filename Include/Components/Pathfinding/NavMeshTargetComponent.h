#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class NavMeshTargetTag
	{
	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshTargetTag);
	};
}
