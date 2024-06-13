#pragma once
#include "Meta/MetaReflect.h"
#include "BasicDataTypes/Colors/LinearColor.h"

namespace CE
{
	class AmbientLightComponent
	{
	public:
		LinearColor mColor = { 1.0f, 1.0f, 1.0f, 1.f };
		float mIntensity = 1.0f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AmbientLightComponent);
	};
}

