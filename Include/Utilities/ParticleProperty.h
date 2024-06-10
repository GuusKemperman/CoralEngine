#pragma once
#include "Math.h"
#include "Random.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Meta/MetaType.h"
#include "Reflect/ReflectFieldType.h"

#include "BasicDataTypes/Colors/LinearColor.h"
#include <cereal/types/array.hpp>
#include <cereal/types/optional.hpp>

namespace CE
{
	template<typename T>
	struct ParticleProperty
	{
		static constexpr size_t sNumOfSamples = 8;

		void SetInitialValuesOfNewParticles(const ParticleEmitterComponent& emitter);
		T GetValue(const ParticleEmitterComponent& emitter, size_t particleIndex) const;

		void DisplayWidget(const std::string& name);

		bool operator==(const ParticleProperty& other) const;
		bool operator!=(const ParticleProperty& other) const;

		T mInitialMin{};
		T mInitialMax{};

		struct ChangeOverTime
		{
			bool operator==(const ChangeOverTime& other) const;

			struct ValueAtTime
			{
				bool operator==(const ValueAtTime& other) const;

				float mTime = -1.0f;
				T mValue{};

			private:
				friend cereal::access;
				template<class Archive>
				void serialize(Archive& ar);
			};

			std::array<ValueAtTime, sNumOfSamples> mMinPoints{};

			struct WithLerp
			{
				bool operator==(const WithLerp& other) const;

				std::array<ValueAtTime, sNumOfSamples> mPoints{};
				std::vector<float> mLerpTime{};


			private:
				friend cereal::access;
				template<class Archive>
				void serialize(Archive& ar);
			};
			std::optional<WithLerp> mMax{};

		private:
			friend cereal::access;
			template<class Archive>
			void serialize(Archive& ar);
		};
		std::optional<ChangeOverTime> mChangeOverTime{};

		std::vector<T> mInitialValues{};

	private:
		static constexpr T GetValue(const std::array<typename ChangeOverTime::ValueAtTime, sNumOfSamples>& points, float t)
		{
			for (size_t i = 0; i < sNumOfSamples - 1; i++)
			{
				if (points[i + 1].mTime < t)
				{
					continue;
				}

				const float weight = Math::lerpInv(points[i].mTime, points[i + 1].mTime, t);
				return Math::lerp(points[i].mValue, points[i + 1].mValue, weight);
			}

			return points.back().mValue;
		}

		friend ReflectAccess;
		static MetaType Reflect();
	};

	template <typename T>
	void ParticleProperty<T>::SetInitialValuesOfNewParticles(const ParticleEmitterComponent& emitter)
	{
		mInitialValues.resize(emitter.GetNumOfParticles());
		const Span<const size_t> newParticles = emitter.GetParticlesThatSpawnedDuringLastStep();

		for (size_t i = 0; i < newParticles.size(); i++)
		{
			const size_t particle = newParticles[i];

			const float t = Random::Value<float>();
			mInitialValues[particle] = Math::lerp(mInitialMin, mInitialMax, t);
		}

		if (mChangeOverTime.has_value()
			&& mChangeOverTime->mMax.has_value())
		{
			mChangeOverTime->mMax->mLerpTime.resize(emitter.GetNumOfParticles());

			for (size_t i = 0; i < newParticles.size(); i++)
			{
				const size_t particle = newParticles[i];
				mChangeOverTime->mMax->mLerpTime[particle] = Random::Value<float>();
			}
		}
	}

	template <typename T>
	T ParticleProperty<T>::GetValue(const ParticleEmitterComponent& emitter, size_t particleIndex) const
	{
		const T& initialValue = mInitialValues[particleIndex];

		if (!mChangeOverTime.has_value())
		{
			return initialValue;
		}
		const float sampleTime = emitter.GetParticleLifeTimesAsPercentage()[particleIndex];

		const T minSample = GetValue(mChangeOverTime->mMinPoints, sampleTime);

		if (!mChangeOverTime->mMax.has_value())
		{
			return minSample * initialValue;
		}

		const T maxSample = GetValue(mChangeOverTime->mMax->mPoints, sampleTime);

		const float lerpValue = mChangeOverTime->mMax->mLerpTime[particleIndex];
		return Math::lerp(minSample, maxSample, lerpValue) * initialValue;
	}

	template <typename T>
	void ParticleProperty<T>::DisplayWidget(const std::string&)
	{
		//ShowInspectUI("mInitialMin", mInitialMin);
		//ShowInspectUI("mInitialMax", mInitialMax);

		bool hasValue = mChangeOverTime.has_value();

		const auto& showPoints = [](const auto& points)
			{
				for (size_t i = 0; i < points.size(); i++)
				{
					ImGui::PushID(static_cast<int>(i));

					//ShowInspectUI("mTime", points[i].mTime);

					ImGui::SameLine();

					//ShowInspectUI("mValue", points[i].mValue);

					ImGui::PopID();
				}
			};

		if (ImGui::TreeNode("mChangeOverTime"))
		{
			if (ImGui::Checkbox("HasValue", &hasValue))
			{
				if (mChangeOverTime.has_value())
				{
					mChangeOverTime.reset();
				}
				else
				{
					mChangeOverTime.emplace();
				}
			}

			if (mChangeOverTime.has_value())
			{
				showPoints(mChangeOverTime->mMinPoints);

				if (ImGui::TreeNode("RandomInRange"))
				{
					if (ImGui::Checkbox("HasValue", &hasValue))
					{
						if (mChangeOverTime->mMax.has_value())
						{
							mChangeOverTime->mMax.reset();
						}
						else
						{
							mChangeOverTime->mMax.emplace();
						}
					}

					if (mChangeOverTime->mMax.has_value())
					{
						showPoints(mChangeOverTime->mMax->mPoints);
					}

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}
	}

	template <typename T>
	bool ParticleProperty<T>::operator==(const ParticleProperty& other) const
	{
		return mInitialMin == other.mInitialMin
			&& mInitialMax == other.mInitialMax
			&& mChangeOverTime == other.mChangeOverTime;
	}

	template <typename T>
	bool ParticleProperty<T>::operator!=(const ParticleProperty& other) const
	{
		return !(*this == other);
	}

	template <typename T>
	bool ParticleProperty<T>::ChangeOverTime::operator==(const ChangeOverTime& other) const
	{
		return mMinPoints == other.mMinPoints
			&& mMax == other.mMax;
	}

	template <typename T>
	bool ParticleProperty<T>::ChangeOverTime::ValueAtTime::operator==(const ValueAtTime& other) const
	{
		return mTime == other.mTime
			&& mValue == other.mValue;
	}

	template <typename T>
	template <class Archive>
	void ParticleProperty<T>::ChangeOverTime::ValueAtTime::serialize(Archive& ar)
	{
		ar(mTime, mValue);
	}

	template <typename T>
	bool ParticleProperty<T>::ChangeOverTime::WithLerp::operator==(const WithLerp& other) const
	{
		return mPoints == other.mPoints;
	}

	template <typename T>
	template <class Archive>
	void ParticleProperty<T>::ChangeOverTime::WithLerp::serialize(Archive& ar)
	{
		uint8 version = 0;
		ar(version, mPoints);
	}

	template <typename T>
	template <class Archive>
	void ParticleProperty<T>::ChangeOverTime::serialize(Archive& ar)
	{
		uint8 version = 0;
		ar(version, mMinPoints, mMax);
	}
}

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<typename T>, CE::ParticleProperty<T>, var.DisplayWidget(name); )
#endif // EDITOR

namespace cereal
{



	template<class Archive, typename T>
	void serialize(Archive& ar, CE::ParticleProperty<T>& value)
	{
		uint8 version = 0;
		ar(version, value.mInitialMin, value.mInitialMax, value.mChangeOverTime);
	}
}

namespace CE
{
	template <typename T>
	MetaType ParticleProperty<T>::Reflect()
	{
		const MetaType& basedOnType = MetaManager::Get().GetType<T>();
		MetaType type{ MetaType::T<ParticleProperty<T>>{}, Format("Particle property {}", basedOnType.GetName()) };

		type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

		type.AddField(&ParticleProperty<T>::mInitialMin, "mInitialMin").GetProperties().Add(Props::sIsScriptableTag);
		type.AddField(&ParticleProperty<T>::mInitialMax, "mInitialMax").GetProperties().Add(Props::sIsScriptableTag);

		ReflectFieldType<ParticleProperty<T>>(type);

		return type;
	}
}





// Particle colour (LinearColor)
// Particle light radius (float)
// Particle light intensity (float)
// Particle mass (float)
// Particle scale (vec3)
