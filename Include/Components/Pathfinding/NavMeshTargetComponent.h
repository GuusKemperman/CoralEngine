#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class NavMeshTargetTag
	{
	public:
		NavMeshTargetTag();

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshTargetTag);
	};
}
