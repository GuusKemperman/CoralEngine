#pragma once
#include "Random.h"
#include "Components/Particles/ParticleEmitterComponent.h"

namespace CE
{
	template<typename T, size_t NumOfSamples = 8>
	struct ParticleProperty
	{
		void SetInitialValuesOfNewParticles(const ParticleEmitterComponent& emitter);
		T GetValue(const ParticleEmitterComponent& emitter, size_t particleIndex);

		T mInitialMin{};
		T mInitialMax{};

		struct ChangeOverTime
		{
			struct ValueAtTime
			{
				float mTime = -1.0f;
				T mValue{};
			};

			std::array<ValueAtTime, NumOfSamples> mMin{};

			struct WithLerp
			{
				std::array<ValueAtTime, NumOfSamples> mMax{};
				std::vector<float> mLerpTime{};
			};
			std::optional<WithLerp> mMax{};

		};
		std::optional<ChangeOverTime> mChangeOverTime{};

		std::vector<T> mInitialValues{};

	private:
		static constexpr T GetSplineValue(const std::array<typename ChangeOverTime::ValueAtTime, NumOfSamples>& spline, float t)
		{
			int num{};

			for (; num < NumOfSamples; num++)
			{
				if (spline[num].mTime < 0.0f
					|| spline[num].mTime > 1.0f)
				{
					break;
				}
			}

			static constexpr signed char coefs[16] = 
			{
				-1, 2,-1, 0,
				3,-5, 0, 2,
				-3, 4, 1, 0,
				1,-1, 0, 0 };

			// find key
			int k = 0;

			while (spline[k].mTime < t)
			{
				k++;
			}
			// interpolant
			const float h = (t - spline[k - 1].mTime) / (spline[k].mTime - spline[k - 1].mTime);

			T v0{};

			// add basis functions
			for (int i = 0; i < 4; i++)
			{
				int kn = glm::clamp(k + i - 2, 0, num - 1);

				const signed char* co = coefs + 4 * i;

				const float b = 0.5f * (((co[0] * h + co[1]) * h + co[2]) * h + co[3]);

				v0 += b * spline[kn].mValue;
			}

			return v0;
		}
	};

	template <typename T, size_t NumOfSamples>
	void ParticleProperty<T, NumOfSamples>::SetInitialValuesOfNewParticles(const ParticleEmitterComponent& emitter)
	{
		mInitialValues.resize(emitter.GetNumOfParticles());
		const Span<const size_t> newParticles = emitter.GetParticlesThatSpawnedDuringLastStep();

		for (size_t i = 0; i < newParticles.size(); i++)
		{
			const size_t particle = newParticles[i];

			const float t = Random::Value<float>();
			mInitialValues[particle] = Math::lerp(mInitialMin, mInitialMax, t);
		}

		if (mChangeOverTime.has_value())
		{
			mChangeOverTime->mLerpTime.resize(emitter.GetNumOfParticles());

			for (size_t i = 0; i < newParticles.size(); i++)
			{
				const size_t particle = newParticles[i];
				mChangeOverTime->mLerpTime[particle] = Random::Value<float>();
			}
		}
	}

	template <typename T, size_t NumOfSamples>
	T ParticleProperty<T, NumOfSamples>::GetValue(const ParticleEmitterComponent& emitter, size_t particleIndex)
	{
		const T& initialValue = mInitialValues[particleIndex];

		if (!mChangeOverTime.has_value())
		{
			return initialValue;
		}
		const float sampleTime = emitter.GetParticleLifeTimesAsPercentage()[particleIndex];

		const T minSample = GetValue(mChangeOverTime->mMin, sampleTime);
		const T maxSample = GetValue(mChangeOverTime->mMin, sampleTime);

		const float lerpValue = mChangeOverTime->mLerpTime[particleIndex]; 
		return Math::lerp(minSample, maxSample, lerpValue) * initialValue;
	}
}



// Particle colour (LinearColor)
// Particle light radius (float)
// Particle light intensity (float)
// Particle mass (float)
// Particle scale (vec3)
