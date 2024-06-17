#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class ToneMappingComponent
	{
	public:
		float mExposure;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ToneMappingComponent);
	};
}

