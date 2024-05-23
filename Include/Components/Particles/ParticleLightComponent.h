#pragma once
#include "BasicDataTypes/Colors/LinearColor.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class ParticleLightComponent
	{
	public:
		Span<const float> GetParticleLightIntensities() const { return mParticleLightIntensities; }

		float mMinLightIntensity = 1.f;
		float mMaxLightIntensity = 1.f;
		float mLightRadius = 1.f;

	private:
		friend class ParticleLightSystem;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleLightComponent);

		std::vector<float> mParticleLightIntensities{};
	};
}
