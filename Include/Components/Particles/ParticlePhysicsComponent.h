#pragma once
#include "Assets/Texture.h"
#include "Meta/MetaReflect.h"
#include "Components/Particles/ParticleUtilities/ParticleUtilitiesFwd.h"

namespace CE
{
	class ParticlePhysicsComponent
	{
	public:
		std::span<glm::vec3> GetLinearVelocities() { return mLinearVelocities; }
		std::span<const glm::vec3> GetLinearVelocities() const { return mLinearVelocities; }

		std::span<glm::quat> GetRotationalVelocitiesPerStep() { return mRotationalVelocitiesPerStep; }
		std::span<const glm::quat> GetRotationalVelocitiesPerStep() const { return mRotationalVelocitiesPerStep; }

		glm::vec3 mMinInitialVelocity{};
		glm::vec3 mMaxInitialVelocity{};

		glm::vec3 mMinInitialRotationalVelocity{};
		glm::vec3 mMaxInitialRotationalVelocity{};

		ParticleProperty<float> mMass{ 1.0f };

		glm::vec3 mGravity{ 1.0f };
		float mFloorHeight{ -std::numeric_limits<float>::infinity() };

	private:
		friend class ParticlePhysicsSystem;
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticlePhysicsComponent);

		std::vector<glm::quat> mRotationalVelocitiesPerStep{};
		std::vector<glm::vec3> mLinearVelocities{};
	};
}
