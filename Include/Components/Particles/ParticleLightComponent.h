#pragma once
#include "Meta/MetaReflect.h"
#include "Components/Particles/ParticleUtilities/ParticleUtilitiesFwd.h"

namespace CE
{
	class ParticleLightComponent
	{
	public:
		ParticleProperty<float> mIntensity{ 1.0f };
		ParticleProperty<float> mRadius{ 1.0f };

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleLightComponent);
	};
}
