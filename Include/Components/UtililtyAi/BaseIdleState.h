#pragma once
#include "Systems/System.h"

namespace Engine
{
	class BaseIdleState
	{
	public:
		BaseIdleState()
		{
		};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(BaseIdleState);
	};
}
