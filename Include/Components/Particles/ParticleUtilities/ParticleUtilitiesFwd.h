#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class ParticleEmitterComponent;

	template<typename T>
	struct ParticleProperty
	{
		ParticleProperty();

		ParticleProperty(T&& value);

		ParticleProperty(T&& min, T&& max);

		void SetInitialValuesOfNewParticles(const ParticleEmitterComponent& emitter);
		T GetValue(const ParticleEmitterComponent& emitter, size_t particleIndex) const;

#ifdef EDITOR
		void DisplayWidget(const std::string& name);
#endif // EDITOR

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

				float mTime = 1.0f;
				T mValue{};

			private:
				friend cereal::access;
				template<class Archive>
				void serialize(Archive& ar);
			};

			std::vector<ValueAtTime> mMinPoints{ ValueAtTime{ 0.0f }, ValueAtTime{ 1.0f } };;

			struct WithLerp
			{
				bool operator==(const WithLerp& other) const;

				std::vector<ValueAtTime> mPoints{ ValueAtTime{ 0.0f }, ValueAtTime{ 1.0f } };
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
		static constexpr T GetValue(const std::vector<typename ChangeOverTime::ValueAtTime>& points, float t);

		friend ReflectAccess;
		static MetaType Reflect();
	};

	template<typename ComponentType>
	void ReflectParticleComponentType(MetaType& type);
}
