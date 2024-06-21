#include "Precomp.h"
#include "Systems/Particles/ParticleLifeTimeSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/TransformComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleEmitterShapes.h"
#include "Components/Particles/ParticleUtilities.h"
#include "Meta/MetaType.h"
#include "Utilities/Random.h"

void CE::ParticleLifeTimeSystem::Update(World& world, float dt)
{
// #define LOG_NUM_OF_PARTICLES

	[[maybe_unused]] size_t totalNumOfAliveParticles{};
	[[maybe_unused]] size_t numberOfEmittersWithShapes{};

	totalNumOfAliveParticles += UpdateEmitters<ParticleEmitterShapeAABB>(world, dt, numberOfEmittersWithShapes);
	totalNumOfAliveParticles += UpdateEmitters<ParticleEmitterShapeSphere>(world, dt, numberOfEmittersWithShapes);

	[[maybe_unused]] size_t numberOfTotalEmitters = world.GetRegistry().View<ParticleEmitterComponent>().size();

	if (numberOfEmittersWithShapes < numberOfTotalEmitters)
	{
		LOG(Particles, Warning, "{} emitter(s) did not have a ParticleEmitterShape component. Atleast one shaper per emitter is required", numberOfTotalEmitters - numberOfEmittersWithShapes);
	}

	if (numberOfEmittersWithShapes > numberOfTotalEmitters)
	{
		LOG(Particles, Warning, "{} emitter(s) have multiple ParticleEmitterShape components. Only one shape per emitter is supported.", numberOfEmittersWithShapes - numberOfTotalEmitters);
	}

#ifdef LOG_NUM_OF_PARTICLES
	LOG(Particles, Verbose, "Num of particles: {}", totalNumOfAliveParticles);
#endif
}

template <typename SpawnShapeType>
size_t CE::ParticleLifeTimeSystem::UpdateEmitters(World& world, float dt, size_t& numOfEmittersFound)
{
#ifdef LOG_NUM_OF_PARTICLES
	size_t totalNumOfAliveParticles{};
#endif // LOG_NUM_OF_PARTICLES

	Registry& reg = world.GetRegistry();

	const auto emitterView = reg.View<ParticleEmitterComponent, const TransformComponent, SpawnShapeType>();

	static constexpr auto onParticleSpawn = [](ParticleEmitterComponent& emitter,
		SpawnShapeType& shape,
		uint32 particleIndex,
		glm::quat emitterOrientation,
		const glm::mat4& emitterMatrix)
		{
			const float lifeTime = Random::Range(emitter.mMinLifeTime, emitter.mMaxLifeTime);
			emitter.mParticleLifeSpan[particleIndex] = lifeTime;
			emitter.mParticleTimeAsPercentage[particleIndex] = 0.0f;

			emitter.mParticlesSpawnedDuringLastStep.push_back(particleIndex);
			shape.OnParticleSpawn(emitter, particleIndex, emitterOrientation, emitterMatrix);
		};

	for (auto [entity, emitter, transform, spawnShape] : emitterView.each())
	{
		numOfEmittersFound++;

		emitter.mParticlesSpawnedDuringLastStep.clear();

		if (emitter.mIsPaused)
		{
			continue;
		}

		if (!emitter.IsPlaying()
			&& emitter.mLoop)
		{
			emitter.PlayFromStart();
		}

		emitter.mEmitterWorldMatrix = transform.GetWorldMatrix();
		emitter.mInverseEmitterWorldMatrix = glm::inverse(emitter.mEmitterWorldMatrix);
		emitter.mEmitterOrientation = transform.GetWorldOrientation();
		emitter.mInverseEmitterOrientation = glm::inverse(emitter.mEmitterOrientation);

		uint32 numToSpawnThisFrame{};
		const glm::mat4 spawnMatrix = emitter.mAreTransformsRelativeToEmitter ? glm::mat4{ 1.0f } : emitter.mEmitterWorldMatrix;
		const glm::quat spawnOrientation = emitter.mAreTransformsRelativeToEmitter ? glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f } : emitter.mEmitterOrientation;

		if (emitter.mOnlyStayAliveUntilExistingParticlesAreGone)
		{
			if (emitter.GetNumOfParticles() == 0)
			{
				reg.Destroy(entity, true);
			}
		}
		else if (emitter.IsPlaying())
		{
			const float totalSpawnRateSurface = emitter.mParticleSpawnRateOverTime.GetSurfaceAreaBetween(0.0f, 1.0f, .05f);

			const float t1 = emitter.mCurrentTime / emitter.mDuration;
			const float dtAsEmitterLifeTimePercentage = dt / emitter.mDuration;
			const float t2 = std::min(t1 + dtAsEmitterLifeTimePercentage, 1.0f);

			const float surfaceAreaBetweenLastStepAndNow = emitter.mParticleSpawnRateOverTime.GetSurfaceAreaBetweenFast(t1, t2);

			emitter.mNumOfParticlesToSpawnNextFrame += (surfaceAreaBetweenLastStepAndNow / totalSpawnRateSurface) * static_cast<float>(emitter.mNumOfParticlesToSpawn);
			const float numToSpawnAsFloat = floorf(emitter.mNumOfParticlesToSpawnNextFrame);

			if (numToSpawnAsFloat > 0.0f)
			{
				emitter.mNumOfParticlesToSpawnNextFrame -= numToSpawnAsFloat;
				numToSpawnThisFrame = static_cast<uint32>(numToSpawnAsFloat);
			}
		}

		emitter.mCurrentTime += dt;

		// Recyle the particles we killed the previous frame
		for (uint32 i = 0; numToSpawnThisFrame > 0 && i < emitter.mParticlesThatDiedDuringLastStep.size(); i++, numToSpawnThisFrame--)
		{
			onParticleSpawn(emitter,
				spawnShape,
				emitter.mParticlesThatDiedDuringLastStep[i],
				spawnOrientation,
				spawnMatrix);
		}
		emitter.mParticlesThatDiedDuringLastStep.clear();

		const uint32 numOfParticlesAliveBeforeLifeTimeUpdate = emitter.GetNumOfParticles();

		int32 indexOfLastParticleInUse{ -1 };

		// Kill/spawn particles
		for (uint32 i = 0; i < numOfParticlesAliveBeforeLifeTimeUpdate; i++)
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
					indexOfLastParticleInUse = static_cast<int32>(i);
				}
				else if (numToSpawnThisFrame > 0)
				{
					onParticleSpawn(emitter, spawnShape, i, spawnOrientation, spawnMatrix);
					indexOfLastParticleInUse = static_cast<int32>(i);
					--numToSpawnThisFrame;
				}

				// The particle was neither respawned, nor did we kill it this frame.
			}
			else
			{
				indexOfLastParticleInUse = static_cast<int32>(i);
			}
		}

		const uint32 minPoolSize = static_cast<uint32>(indexOfLastParticleInUse + 1) + numToSpawnThisFrame;

		emitter.mParticlePositions.resize(minPoolSize);
		emitter.mParticleOrientations.resize(minPoolSize);
		emitter.mParticleTimeAsPercentage.resize(minPoolSize);
		emitter.mParticleLifeSpan.resize(minPoolSize);

		for (uint32 i = 0; i < numToSpawnThisFrame; i++)
		{
			onParticleSpawn(emitter,
				spawnShape,
				numOfParticlesAliveBeforeLifeTimeUpdate + i,
				spawnOrientation,
				spawnMatrix);
		}

		emitter.mScale.SetInitialValuesOfNewParticles(emitter);

		if (!emitter.IsPlaying()
			&& emitter.mDestroyOnFinish)
		{
			emitter.mDestroyOnFinish = false;
			reg.Destroy(entity, true);
		}

#ifdef LOG_NUM_OF_PARTICLES
		for (size_t i = 0; i < emitter.mParticleTimeAsPercentage.size(); i++)
		{
			totalNumOfAliveParticles += emitter.mParticleTimeAsPercentage[i] <= 1.0f;
		}
#endif // LOG_NUM_OF_PARTICLES
	}

#ifdef LOG_NUM_OF_PARTICLES
	return totalNumOfAliveParticles;
#else
	return 0;
#endif // LOG_NUM_OF_PARTICLES
}

CE::MetaType CE::ParticleLifeTimeSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleLifeTimeSystem>{}, "ParticleLifeTimeSystem", MetaType::Base<System>{} };
}
