#pragma once
#include "Assets/Texture.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class ParticlePhysicsComponent
	{
	public:
		Span<glm::vec3> GetLinearVelocities() { return mLinearVelocities; }
		Span<const glm::vec3> GetLinearVelocities() const { return mLinearVelocities; }

		Span<glm::quat> GetRotationalVelocitiesPerStep() { return mRotationalVelocitiesPerStep; }
		Span<const glm::quat> GetRotationalVelocitiesPerStep() const { return mRotationalVelocitiesPerStep; }

		Span<float> GetMasses() { return mParticleMasses; }
		Span<const float> GetMasses() const { return mParticleMasses; }
		
		
		glm::vec3 mMinInitialVelocity{};
		glm::vec3 mMaxInitialVelocity{};

		
		glm::vec3 mMinInitialRotationalVelocity{};
		glm::vec3 mMaxInitialRotationalVelocity{};

		
		float mMinMass{};
		float mMaxMass{};

		glm::vec3 mGravity{};
		float mFloorHeight{};

	private:
		friend class ParticlePhysicsSystem;
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticlePhysicsComponent);

		std::vector<glm::quat> mRotationalVelocitiesPerStep{};
		std::vector<glm::vec3> mLinearVelocities{};
		std::vector<float> mParticleMasses{};
	};
}
