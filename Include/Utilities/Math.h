#pragma once


constexpr float PI = 3.1415927358979323848f;
constexpr float INVPI = 0.31830988618379067153777f;
constexpr float INV2PI = 0.15915494309189533576888f;
constexpr float TWOPI = 6.28318530717958647692528f;
constexpr float SQRT_PI_INV = 0.56418958355f;
constexpr float LARGE_FLOAT = 1e34f;

namespace CE
{
	class Math
	{
	public:
		template <class T>
		static void Swap(T& x, T& y) { T t; t = x, x = y, y = t; }

		template <typename T>
		static constexpr T sqr(T v) { return v * v; }

		template <typename T>
		static constexpr T lerp(T min, T max, float t) { return min + t * (max - min); }

		template <typename T>
		static constexpr float lerpInv(T min, T max, T value) { return (value - min) / (max - min); }

		template <typename T>
		static constexpr bool IsPowerOfTwo(T x) { return (x != 0) && ((x & (x - 1)) == 0); }

		// https://www.gamedeveloper.com/business/how-to-work-with-bezier-curve-in-games-with-unity For more info
		template<typename T>
		static constexpr T BezierCurve(const T& p0, const T& q1, const T& q2, const float t)
		{
			return (1.0f - t) * 2.0f * p0 + 2.0f * (1.0f - t) * t * q1 + t * 2.0f * q2;
		}

		// In radians
		static float Vec2ToAngle(glm::vec2 vec)
		{
			return std::atan2(vec.y, vec.x);
		}
		// In radians
		static constexpr glm::vec2 AngleToVec2(float radian)
		{
			return { std::cos(radian), std::sin(radian) };
		}

		//static glm::vec2 GLFWPixelToClipSpace(const glm::ivec2& pixelPosition);
		//static inline glm::vec2 OpenGLPixelToClipspace(const glm::ivec2& pixelPosition)
		//{
		//	return {
		//		(2.0 * pixelPosition.x + 1.0) / sScreenWidth - 1.0,
		//		(2.0 * pixelPosition.y + 1.0) / sScreenHeight - 1.0
		//	};
		//}

		// Stolen from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternions/
		static glm::quat CalculateOrientationTowards(glm::quat current, const glm::quat& target, float maxAngle);

		static glm::quat CalculateRotationBetweenOrientations(glm::vec3 start, glm::vec3 dest);

		static glm::quat CalculateRotationBetweenOrientations(glm::quat start, glm::quat end);

		// Stolen from https://stackoverflow.com/questions/44705398/about-glm-quaternion-rotation
		static glm::vec3 RotateVector(const glm::vec3& v, const glm::quat& q);

		static constexpr uint32 floorlog2(uint32 x)
		{
			return x == 1 ? 0 : 1 + floorlog2(x >> 1);
		}

		static constexpr uint32 ceillog2(uint32 x)
		{
			return x == 1 ? 0 : floorlog2(x - 1) + 1;
		}

		template<typename VecType, glm::length_t Size>
		static constexpr glm::vec<Size, VecType> ClampLength(const glm::vec<Size, VecType>& vec, const VecType& min, const VecType& max)
		{
			const float length2 = glm::length2(vec);

			if (length2 == 0.0f)
			{
				return glm::vec<Size, VecType>{ 0.0f };
			}

			if (length2 < min * min)
			{
				return (vec / sqrtf(length2)) * min;
			}
			else if (length2 > max * max)
			{
				return (vec / sqrtf(length2)) * max;
			}
			return vec;
		}

		static glm::vec2 QuatToDirectionXZ(const glm::quat& quat)
		{
			const glm::vec3 direction3D = glm::vec3(0.f, 0.f, 1.f) * quat;
			return normalize(glm::vec2(-direction3D.x, direction3D.z));
		}

		static glm::vec3 QuatToDirection(const glm::quat& quat)
		{
			const glm::vec3 direction3D = glm::vec3(0.f, 0.f, 1.f) * quat;
			return normalize(glm::vec3(-direction3D.x, direction3D.y, direction3D.z));
		}

		static glm::quat Direction2DToXZQuatOrientation(glm::vec2 v)
		{
			const glm::vec2 direction2D = glm::normalize(v);
			const glm::vec3 direction3D(direction2D.x, 0.0f, direction2D.y);
			// Calculate angle based on direction
			float angle = acos(glm::dot(glm::normalize(direction3D), { 0.0f, 0.0f, 1.0f }));
			// Determine the rotation direction (clockwise or counterclockwise)
			if (direction3D.x < 0)
				angle = -angle;
			// Create quaternion for rotation around Y-axis
			const glm::quat rotation = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));

			return rotation;
		}

		static bool AreFloatsEqual(float a, float b, float epsilon = 1e-5f)
		{
			return std::fabs(a - b) < epsilon;
		}
		
		static std::optional<std::vector<glm::vec3>> CalculateTangents(
        const void* const indices,
        const size_t numOfIndices,
        const bool areIndices16Bit,
        const glm::vec3* const positions,
        const glm::vec3* const normals,
        const glm::vec2* const texCoords,
        const size_t numOfVertices);
	};
}