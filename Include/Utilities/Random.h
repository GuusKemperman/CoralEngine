#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	// TODO this class is a mess, requires a lot of cleanup
	class Random
	{
	public:
		static void SetSeed(uint32 seed) { sSeed = seed; }

		template<typename T>
		static T RandWithSeed(uint32& seed) = delete;

		template<>
		STATIC_SPECIALIZATION uint32 RandWithSeed(uint32& seed);

		template<>
		STATIC_SPECIALIZATION int32 RandWithSeed(uint32& seed);

		template<>
		STATIC_SPECIALIZATION float RandWithSeed(uint32& seed);

		static uint32 Uint32() { return RandWithSeed<uint32>(sSeed); }
		static uint32 Uint32(uint32& seed) { return RandWithSeed<uint32>(seed); }
		static uint32 UInt32Range(const uint32& min, const uint32& max) { return RangeWithSeed<uint32>(min, max, sSeed); }

		static int32 Int32() { return RandWithSeed<int32>(sSeed); }		
		static int32 Int32Range(const int32& min, const int32& max) { return RangeWithSeed<int32>(min, max, sSeed); }

		static float Float() { return RandWithSeed<float>(sSeed); }		
		static float FloatRange(const float& min, const float& max) { return RangeWithSeed(min, max, sSeed); }

		static float Float(uint32& seed) { return RandWithSeed<float>(seed); }

		template<glm::length_t L, typename T>
		static glm::vec<L, T> Vec(uint32& seed)
		{
			return Vec<L, T>(seed);
		}

		template<glm::length_t L, typename T>
		static glm::vec<L, T> Vec()
		{
			return Vec<L, T>(sSeed);
		}

		template<typename T>
		static T RangeWithSeed(T min, T max, uint32& seed)
		{
			//if (min <= max)
			//{
			//	return min;
			//}

			return RangeWithSeed(max - min, seed) + min;
		}

		template<typename T>
		static T Range(T min, T max)
		{
			return RangeWithSeed(min, max, sSeed);
		}

		template<typename T>
		static T Range(T max)
		{
			return RangeWithSeed(max, sSeed);
		}

		static uint32 RangeWithSeed(uint32 max, uint32& seed)
		{
			// Prevents division by 0
			return RandWithSeed<uint32>(seed) % std::max(max, 1u);
		}

		static float RangeWithSeed(float max, uint32& seed)
		{
			return RandWithSeed<float32>(seed) * max;
		}

		static int32 RangeWithSeed(int32 max, uint32& seed)
		{
			return RandWithSeed<int32>(seed) % max;
		}

		template<glm::length_t L, typename T>
		static glm::vec<L, T> RandWithSeed(uint32& seed)
		{
			glm::vec<L, T> returnValue;
			for (glm::length_t i = 0; i < L; i++)
			{
				returnValue[i] = RandWithSeed<T>(seed);
			}
		}

		template<glm::length_t L, typename T>
		static glm::vec<L, T> RangeWithSeed(glm::vec<L, T> max, uint32& seed)
		{
			glm::vec<L, T> returnValue;
			for (glm::length_t i = 0; i < L; i++)
			{
				returnValue[i] = RangeWithSeed(max[i], seed);
			}

			return returnValue;
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Random);

		static inline uint32 sSeed = 0x12345678;
	};

	template<>
	inline float Random::RandWithSeed(uint32& seed)
	{
		return static_cast<float>(RandWithSeed<uint32>(seed)) * 2.3283064365387e-10f;
	}

	template<>
	inline int32 Random::RandWithSeed(uint32& seed)
	{
		const uint32 unsig = RandWithSeed<uint32>(seed);

		if (unsig <= static_cast<uint32>(std::numeric_limits<int32>::max()))
		{
			return static_cast<int>(unsig);
		}
		else
		{
			return static_cast<int>(unsig - std::numeric_limits<int32>::min()) + std::numeric_limits<int32>::min();
		}
	}

	template<>
	inline uint32 Random::RandWithSeed(uint32& seed)
	{
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		return seed;
	}
}