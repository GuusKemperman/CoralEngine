#pragma once
#include "BasicDataTypes/Colors/ColorGradient.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class ParticleColorOverTimeComponent
	{
	public:
		ColorGradient mGradient{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleColorOverTimeComponent);
	};
}
