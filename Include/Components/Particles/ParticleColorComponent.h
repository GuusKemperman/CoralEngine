#pragma once
#include "BasicDataTypes/Colors/LinearColor.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class ParticleColorComponent
	{
	public:
		Span<LinearColor> GetColors() { return mParticleColors; }
		Span<const LinearColor> GetColors() const { return mParticleColors; }

		LinearColor mMinParticleColor{ 1.0f };
		LinearColor mMaxParticleColor{ 1.0f };

	private:
		friend class ParticleColorSystem;
		
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleColorComponent);

		std::vector<LinearColor> mParticleColors{};
	};
}
