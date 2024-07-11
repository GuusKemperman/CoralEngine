#pragma once
#include "BasicDataTypes/Bezier.h"
#include "Meta/MetaReflect.h"
#include "Components/Particles/ParticleUtilities/ParticleUtilitiesFwd.h"

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

		glm::mat4 GetParticleMatrixFast(uint32 particle) const;
		glm::mat4 GetParticleMatrixWorld(uint32 particle) const;

		glm::vec3 GetParticlePositionFast(uint32 particle) const;
		void SetParticlePositionFast(uint32 particle, glm::vec3 position);

		glm::vec3 GetParticlePositionWorld(uint32 particle) const;
		void SetParticlePositionWorld(uint32 particle, glm::vec3 position);

		glm::vec3 GetParticleScaleFast(uint32 particle) const;

		// Reaallyy slow
		glm::vec3 GetParticleScaleWorld(uint32 particle) const;

		glm::quat GetParticleOrientationFast(uint32 particle) const;
		void SetParticleOrientationFast(uint32 particle, glm::quat orientation);

		glm::quat GetParticleOrientationWorld(uint32 particle) const;
		void SetParticleOrientationWorld(uint32 particle, glm::quat orientation);

		Span<float> GetParticleLifeTimesAsPercentage() { return mParticleTimeAsPercentage; }
		Span<const float> GetParticleLifeTimesAsPercentage() const { return mParticleTimeAsPercentage; }

		Span<float> GetParticleLifeSpans() { return mParticleLifeSpan; }
		Span<const float> GetParticleLifeSpans() const { return mParticleLifeSpan; }

		Span<const uint32> GetParticlesThatSpawnedDuringLastStep() const { return mParticlesSpawnedDuringLastStep; }
		Span<const uint32> GetParticlesThatDiedDuringLastStep() const { return mParticlesThatDiedDuringLastStep; }

		void PlayFromStart();

		uint32 mNumOfParticlesToSpawn{ 1000 };
		Bezier mParticleSpawnRateOverTime{};

		float mMinLifeTime = 1.0f;
		float mMaxLifeTime = 10.0f;

		bool mAreTransformsRelativeToEmitter{};

		bool mLoop = true;
		bool mKeepExistingParticlesAliveWhenRestartingLoop = true;
		bool mDestroyOnFinish = false;
		bool mIsPaused = false;
		float mDuration = 10.0f;
		float mCurrentTime{};

		glm::mat4 mEmitterWorldMatrix = glm::mat4{ 1.0f };
		glm::mat4 mInverseEmitterWorldMatrix = glm::inverse(mEmitterWorldMatrix);

		glm::quat mEmitterOrientation = { 1.0f, 0.0f, 0.0f, 0.0f };
		glm::quat mInverseEmitterOrientation = glm::inverse(mEmitterOrientation);

		bool mKeepParticlesAliveWhenEmitterIsDestroyed = true;
		bool mOnlyStayAliveUntilExistingParticlesAreGone = false;

	private:
		friend class ParticleLifeTimeSystem;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ParticleEmitterComponent);

		// We make these vectors private to prevent users from resizing these directly, 
		// but their contents can be modified using the mutable span getters.
		std::vector<glm::vec3> mParticlePositions{};
		ParticleProperty<glm::vec3> mScale{ glm::vec3{ 1.0f } };
		std::vector<glm::quat> mParticleOrientations{};

		std::vector<float> mParticleTimeAsPercentage{};
		std::vector<float> mParticleLifeSpan{};

		std::vector<uint32> mParticlesSpawnedDuringLastStep{};
		std::vector<uint32> mParticlesThatDiedDuringLastStep{};

		// Why is this a float you might ask?
		// If you want to, for example, spawn .5f particles per frame, you can't do anything the first frame, as you can't spawn half a particle.
		// But the next frame, two halves make one whole.
		float mNumOfParticlesToSpawnNextFrame{};
	};
}
