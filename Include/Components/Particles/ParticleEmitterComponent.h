#pragma once
#include "BasicDataTypes/Bezier.h"
#include "Meta/MetaReflect.h"
#include "Components/Particles/ParticleProperty/ParticlePropertyFwd.h"

namespace CE
{
	class ParticleEmitterComponent
	{
	public:

		bool IsPaused() const { return mIsPaused; }
		bool IsPlaying() const { return mCurrentTime <= mDuration; }

		uint32 GetNumOfParticles() const { return static_cast<uint32>(mParticlePositions.size()); }

		bool DidParticleJustSpawn(const uint32 particle) const { return mParticleTimeAsPercentage[particle] == 0.0f; }
		bool IsParticleAlive(const uint32 particle) const { return mParticleTimeAsPercentage[particle] <= 1.0f; }

		Span<glm::vec3> GetParticlePositions() { return mParticlePositions; }
		Span<const glm::vec3> GetParticlePositions() const { return mParticlePositions; }

		Span<glm::quat> GetParticleOrientations() { return mParticleOrientations; }
		Span<const glm::quat> GetParticleOrientations() const { return mParticleOrientations; }

		Span<float> GetParticleLifeTimesAsPercentage() { return mParticleTimeAsPercentage; }
		Span<const float> GetParticleLifeTimesAsPercentage() const { return mParticleTimeAsPercentage; }

		Span<float> GetParticleLifeSpans() { return mParticleLifeSpan; }
		Span<const float> GetParticleLifeSpans() const { return mParticleLifeSpan; }

		Span<const size_t> GetParticlesThatSpawnedDuringLastStep() const { return mParticlesSpawnedDuringLastStep; }
		Span<const size_t> GetParticlesThatDiedDuringLastStep() const { return mParticlesThatDiedDuringLastStep; }

		void PlayFromStart();

		uint32 mNumOfParticlesToSpawn{ 1000 };
		Bezier mParticleSpawnRateOverTime{};

		float mMinLifeTime = 1.0f;
		float mMaxLifeTime = 10.0f;

		bool mLoop = true;
		bool mKeepExistingParticlesAliveWhenRestartingLoop = true;
		bool mDestroyOnFinish = false;
		bool mIsPaused = false;
		float mDuration = 10.0f;
		float mCurrentTime{};

		ParticleProperty<glm::vec3> mScale{ glm::vec3{ 1.0f }  };

	private:
		friend class ParticleLifeTimeSystem;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleEmitterComponent);

		// We make these vectors private to prevent users from resizing these directly, 
		// but their contents can be modified using the mutable span getters.
		std::vector<glm::vec3> mParticlePositions{};
		std::vector<glm::vec3> mParticleScales{};
		std::vector<glm::quat> mParticleOrientations{};

		std::vector<float> mParticleTimeAsPercentage{};
		std::vector<float> mParticleLifeSpan{};

		std::vector<size_t> mParticlesSpawnedDuringLastStep{};
		std::vector<size_t> mParticlesThatDiedDuringLastStep{};

		// Why is this a float you might ask?
		// If you want to, for example, spawn .5f particles per frame, you can't do anything the first frame, as you can't spawn half a particle.
		// But the next frame, two halves make one whole.
		float mNumOfParticlesToSpawnNextFrame{};
	};
}
