#pragma once
#include <random>

#include "Meta/MetaReflect.h"

namespace CE
{
	class Random
	{
	public:
		// Min Inclusive, Max Exclusive
		template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
		static T Range(T min, T max)
		{
			std::uniform_int_distribution distribution{ min, max == min ? max : max - 1 };
			return distribution(sEngine);
		}

		template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
		static T Range(T min, T max)
		{
			std::uniform_real_distribution distribution{ min, max };
			return distribution(sEngine);
		}

		template<glm::length_t L, typename T>
		static glm::vec<L, T> Range(glm::vec<L, T> min, glm::vec<L, T> max)
		{
			glm::vec<L, T> returnValue;
			for (glm::length_t i = 0; i < L; i++)
			{
				returnValue[i] = Range(min[i], max[i]);
			}
			return returnValue;
		}

		template<typename T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, bool> = true>
		static T Value()
		{
			return Range(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
		}

		template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
		static T Value()
		{
			return Range<T>(0, 1);
		}

		template<typename T, std::enable_if_t<std::is_same_v<T, bool>, bool> = true>
		static T Value()
		{
			return Range<uint32>(0, 1);
		}

		static uint32 CreateSeed(glm::vec2 position);

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Random);

		static inline std::random_device sDevice{};
		static inline std::default_random_engine sEngine{sDevice()};
	};
}