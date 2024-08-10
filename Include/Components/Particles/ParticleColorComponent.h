#pragma once
#include "BasicDataTypes/Colors/LinearColor.h"
#include "Meta/MetaReflect.h"
#include "Components/Particles/ParticleUtilities/ParticleUtilitiesFwd.h"

namespace CE
{
	class ParticleColorComponent
	{
	public:
		ParticleProperty<LinearColor> mMultiplicativeColor{ LinearColor{ 1.0f } };
		ParticleProperty<LinearColor> mAdditiveColor{ LinearColor{ 0.0f } };

	private:
		friend class ParticleColorSystem;
		
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleColorComponent);
	};
}
