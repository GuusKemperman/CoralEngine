#include "Precomp.h"
#include "Systems/Particles/ParticlePhysicsSystem.h"

#include "Components/Particles/ParticlePhysicsComponent.h"
#include "Utilities/Random.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleUtilities.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void CE::ParticlePhysicsSystem::Update(World& world, float dt)
{
	const auto emitterView = world.GetRegistry().View<const TransformComponent, ParticleEmitterComponent, ParticlePhysicsComponent>();

	for (auto [entity, transform, emitter, physics] : emitterView.each())
	{
		if (emitter.IsPaused())
		{
			continue;
		}

		const uint32 numOfParticles = emitter.GetNumOfParticles();
		physics.mRotationalVelocitiesPerStep.resize(numOfParticles);
		physics.mLinearVelocities.resize(numOfParticles);
		physics.mMass.SetInitialValuesOfNewParticles(emitter);

		if (emitter.mAreTransformsRelativeToEmitter)
		{
			for (const uint32 particle : emitter.GetParticlesThatSpawnedDuringLastStep())
			{
				// TODO: This only works with fixed time step
				physics.mRotationalVelocitiesPerStep[particle] = glm::quat(Random::Range(physics.mMinInitialRotationalVelocity, physics.mMaxInitialRotationalVelocity) * Particles::sParticleFixedTimeStep.value_or(1.0f / 60.0f));
				physics.mLinearVelocities[particle] = Random::Range(physics.mMinInitialVelocity, physics.mMaxInitialVelocity);
			}
		}
		else
		{
			const glm::quat emitterOrientation = transform.GetWorldOrientation();

			for (const uint32 particle : emitter.GetParticlesThatSpawnedDuringLastStep())
			{
				// TODO: This only works with fixed time step
				physics.mRotationalVelocitiesPerStep[particle] = glm::quat(Random::Range(physics.mMinInitialRotationalVelocity, physics.mMaxInitialRotationalVelocity) * Particles::sParticleFixedTimeStep.value_or(1.0f / 60.0f));
				physics.mLinearVelocities[particle] = Math::RotateVector(Random::Range(physics.mMinInitialVelocity, physics.mMaxInitialVelocity), emitterOrientation);
			}
		}

		const glm::vec3 timeScaledGrav = dt * physics.mGravity;

		// Move the particles
		for (uint32 i = 0; i < numOfParticles; i++)
		{
			// Note that we are also moving the dead particles, but that doesn't matter as they won't get rendered
			physics.mLinearVelocities[i] += timeScaledGrav * physics.mMass.GetValue(emitter, i);

			glm::vec3 position = emitter.GetParticlePositionFast(i);
			glm::quat orientation = emitter.GetParticleOrientationFast(i);

			position += physics.mLinearVelocities[i] * dt;
			position[Axis::Up] = glm::max(position[Axis::Up], physics.mFloorHeight);

			orientation *= physics.mRotationalVelocitiesPerStep[i];

			emitter.SetParticlePositionFast(i, position);
			emitter.SetParticleOrientationFast(i, orientation);
		}

		// Benchmarked, and was slightly slower than combining the loops
		//// Rotate the particles
		//for (size_t i = 0; i < numOfParticles; i++)
		//{
		//	orientations[i] *= physics.mRotationalVelocitiesPerStep[i];
		//}
	}
}

CE::MetaType CE::ParticlePhysicsSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticlePhysicsSystem>{}, "ParticlePhysicsSystem", MetaType::Base<System>{} };
}
