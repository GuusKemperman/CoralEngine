#pragma once
#include "BasicDataTypes/Bezier.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class ParticleScaleOverTimeComponent
	{
	public:
		Bezier mScaleMultiplierOverParticleLifeTime{};

		std::vector<glm::vec3> mParticleInitialScales{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleScaleOverTimeComponent);
	};
}
