#include "Precomp.h"
#include "Systems/Particles/ParticleLifeTimeSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::ParticleLifeTimeSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();

	const auto emitterView = reg.View<ParticleEmitterComponent, const TransformComponent>();

	// #define LOG_NUM_OF_PARTICLES

#ifdef LOG_NUM_OF_PARTICLES
	size_t totalNumOfAliveParticles{};
#endif // LOG_NUM_OF_PARTICLES

	for (auto [entity, emitter, transform] : emitterView.each())
	{
		emitter.mParticlesSpawnedDuringLastStep.clear();

		if (emitter.mIsPaused)
		{
			continue;
		}

		const float totalSpawnRateSurface = emitter.mParticleSpawnRateOverTime.GetSurfaceAreaBetween(0.0f, 1.0f, .05f);
		const glm::mat4 emitterMatrix = transform.GetWorldMatrix();
		const glm::vec3 emitterScale = transform.GetWorldScale();
		const glm::quat emitterOrientation = transform.GetWorldOrientation();

		const float t1 = emitter.mCurrentTime / emitter.mDuration;
		const float dtAsEmitterLifeTimePercentage = dt / emitter.mDuration;
		const float t2 = std::min(t1 + dtAsEmitterLifeTimePercentage, 1.0f);

		const float surfaceAreaBetweenLastStepAndNow = emitter.mParticleSpawnRateOverTime.GetSurfaceAreaBetweenFast(t1, t2);

		emitter.mNumOfParticlesToSpawnNextFrame += (surfaceAreaBetweenLastStepAndNow / totalSpawnRateSurface) * static_cast<float>(emitter.mNumOfParticlesToSpawn);
		const float numToSpawnAsFloat = floorf(emitter.mNumOfParticlesToSpawnNextFrame);
		size_t numToSpawnThisFrame{};

		if (numToSpawnAsFloat > 0.0f)
		{
			emitter.mNumOfParticlesToSpawnNextFrame -= numToSpawnAsFloat;
			numToSpawnThisFrame = static_cast<size_t>(numToSpawnAsFloat);
		}

		emitter.mCurrentTime += dt;
		if (!emitter.IsPlaying())
		{
			if (emitter.mLoop)
			{
				emitter.PlayFromStart();
			}
			else
			{
				if (emitter.mDestroyOnFinish)
				{
					reg.Destroy(entity);
				}
				continue;
			}
		}
		
		// Recyle the particles we killed the previous frame
		for (size_t i = 0; numToSpawnThisFrame > 0 && i < emitter.mParticlesThatDiedDuringLastStep.size(); i++, numToSpawnThisFrame--)
		{
			emitter.OnParticleSpawn(emitter.mParticlesThatDiedDuringLastStep[i], emitterOrientation, emitterScale, emitterMatrix);
		}
		emitter.mParticlesThatDiedDuringLastStep.clear();

		const size_t numOfParticlesAliveBeforeLifeTimeUpdate = emitter.GetNumOfParticles();

		size_t indexOfLastParticleInUse{};

		// Kill/spawn particles
		for (size_t i = 0; i < numOfParticlesAliveBeforeLifeTimeUpdate; i++)
		{
			const float totalLifeSpan = emitter.mParticleLifeSpan[i];
			const float dtAsPercentageOfLifeSpan = dt / totalLifeSpan;

			emitter.mParticleTimeAsPercentage[i] += dtAsPercentageOfLifeSpan;

			// Is the particle dead?
			if (emitter.mParticleTimeAsPercentage[i] > 1.0f)
			{
				// Did we just kill it?
				if (emitter.mParticleTimeAsPercentage[i] < 1.0f + dtAsPercentageOfLifeSpan)
				{
					// We don't want to immediately recycle the particle,
					// other components might need the 'state' of the particle
					// at it's time of death to persist for one more step/frame.
					emitter.mParticlesThatDiedDuringLastStep.push_back(i);
					indexOfLastParticleInUse = i;
				}
				else if(numToSpawnThisFrame > 0)
				{
					emitter.OnParticleSpawn(i, emitterOrientation, emitterScale, emitterMatrix);
					indexOfLastParticleInUse = i;
					--numToSpawnThisFrame;
				}
				
				// The particle was neither respawned, nor did we kill it this frame.
			}
			else
			{
				indexOfLastParticleInUse = i;
			}
		}

		const size_t minPoolSize = indexOfLastParticleInUse + 1 + numToSpawnThisFrame;

		emitter.mParticlePositions.resize(minPoolSize);
		emitter.mParticleScales.resize(minPoolSize);
		emitter.mParticleOrientations.resize(minPoolSize);
		emitter.mParticleTimeAsPercentage.resize(minPoolSize);
		emitter.mParticleLifeSpan.resize(minPoolSize);

		for (size_t i = 0; i < numToSpawnThisFrame; i++)
		{
			emitter.OnParticleSpawn(numOfParticlesAliveBeforeLifeTimeUpdate + i, emitterOrientation, emitterScale, emitterMatrix);
		}



#ifdef LOG_NUM_OF_PARTICLES
		for (size_t i = 0; i < emitter.mParticleTimeAsPercentage.size(); i++) 
		{
			totalNumOfAliveParticles += emitter.mParticleTimeAsPercentage[i] <= 1.0f;
		}
#endif // LOG_NUM_OF_PARTICLES
	}

#ifdef LOG_NUM_OF_PARTICLES
	LOG_FMT(Particles, Verbose, "Num of particles: {}", totalNumOfAliveParticles);
#endif
}

Engine::MetaType Engine::ParticleLifeTimeSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleLifeTimeSystem>{}, "ParticleLifeTimeSystem", MetaType::Base<System>{} };
}
