#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class NavMeshTargetComponent
	{
	public:
		NavMeshTargetComponent();

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(NavMeshTargetComponent);
	};
}
