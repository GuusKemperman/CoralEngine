#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class NavMeshObstacleTag
	{
	public:
		NavMeshObstacleTag();

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshObstacleTag);
	};
}
