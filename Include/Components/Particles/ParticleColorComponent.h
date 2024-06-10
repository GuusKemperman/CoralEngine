#pragma once
#include "BasicDataTypes/Colors/LinearColor.h"
#include "Meta/MetaReflect.h"
#include "Utilities/ParticleProperty.h"

namespace CE
{
	class ParticleColorComponent
	{
	public:
		ParticleProperty<LinearColor> mColor{};
		ParticleProperty<float> mTest{};

	private:
		friend class ParticleColorSystem;
		
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleColorComponent);
	};
}
