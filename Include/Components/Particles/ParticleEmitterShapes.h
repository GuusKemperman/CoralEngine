#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class ParticleEmitterComponent;

	class ParticleEmitterShapeAABB
	{
	public:
		void OnParticleSpawn(ParticleEmitterComponent& emitter, 
			size_t particle, 
			glm::quat emitterWorldOrientation, 
			const glm::mat4& emitterMatrix) const;

		glm::vec3 mMinOrientation{};
		glm::vec3 mMaxOrientation{ 360.0f };

		glm::vec3 mMinPosition{ -1.0f };
		glm::vec3 mMaxPosition{ 1.0f };

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleEmitterShapeAABB);
	};

	class ParticleEmitterShapeSphere
	{
	public:
		void OnParticleSpawn(ParticleEmitterComponent& emitter,
			size_t particle,
			glm::quat emitterWorldOrientation,
			const glm::mat4& emitterMatrix) const;

		glm::vec3 mMinOrientation{};
		glm::vec3 mMaxOrientation{ 360.0f };

		float mRadius = 1.0f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleEmitterShapeSphere);
	};
}
