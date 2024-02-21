#include "Precomp.h"
#include "Utilities/Math.h"

//glm::vec2 Engine::Math::GLFWPixelToClipSpace(const glm::ivec2& pixelPosition)
//{
//	glm::vec2 openGLPos = { pixelPosition.x == 0 ? -1.0f : static_cast<float>(pixelPosition.x % static_cast<int>(sHalfWindowWidth)),
//		pixelPosition.y == 0 ? -1.0f : static_cast<float>(pixelPosition.y % static_cast<int>(sHalfWindowHeight)) };
//
//	if (pixelPosition.x < static_cast<int>(sHalfWindowWidth))
//	{
//		openGLPos.x = openGLPos.x - static_cast<int>(sHalfWindowWidth);
//	}
//
//	if (pixelPosition.y < static_cast<int>(sHalfWindowHeight))
//	{
//		openGLPos.y = openGLPos.y - static_cast<int>(sHalfWindowHeight);
//	}
//
//	if (openGLPos.x != 0.0f)
//	{
//		openGLPos.x /= static_cast<float>(sHalfWindowWidth);
//	}
//
//	if (openGLPos.y != 0.0f)
//	{
//		openGLPos.y /= -static_cast<float>(sHalfWindowHeight);
//	}
//
//	return openGLPos;
//}

// Stolen from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-17-quaternion
glm::quat Engine::Math::CalculateOrientationTowards(glm::quat current, const glm::quat& target, const float maxAngle)
{
	if (maxAngle == 0.0f)
	{
		return current;
	}

	float cosTheta = dot(current, target);
	ASSERT(cosTheta >= -1.0f
		&& cosTheta <= 1.0f
		&& "This value is not in a valid range and will produces NaN's later. May be a floating point error.");

	// q1 and q2 are already equal.
	// Force q2 just to be sure
	if (cosTheta > 0.9999f)
	{
		return target;
	}

	// Avoid taking the long path around the sphere
	if (cosTheta < 0)
	{
		current = current * -1.0f;
		cosTheta *= -1.0f;
	}

	float angle = acos(cosTheta);

	// If there is only a 2&deg; difference, and we are allowed 5&deg;,
	// then we arrived.
	if (angle <= maxAngle)
	{
		return target;
	}

	const float fT = maxAngle / angle;
	angle = maxAngle;

	glm::quat res = (sin((1.0f - fT) * angle) * current + sin(fT * angle) * target) / sin(angle);
	res = normalize(res);
	return res;
}

glm::quat Engine::Math::CalculateRotationBetweenOrientations(glm::vec3 start, glm::vec3 dest)
{
	if (start == glm::vec3(0.0f, 0.0f, 0.0f)
		|| dest == glm::vec3(0.0f, 0.0f, 0.0f))
	{
		return {};
	}

	start = normalize(start);
	dest = normalize(dest);

	const float cosTheta = dot(start, dest);
	glm::vec3 rotationAxis;

	if (cosTheta < -1 + 0.001f)
	{
		// special case when vectors in opposite directions:
		// there is no "ideal" rotation axis
		// So guess one; any will do as long as it's perpendicular to start
		rotationAxis = cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
		if (length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
		{
			rotationAxis = cross(glm::vec3(1.0f, 0.0f, 0.0f), start);
		}

		rotationAxis = normalize(rotationAxis);
		return angleAxis(glm::radians(180.0f), rotationAxis);
	}

	rotationAxis = cross(start, dest);

	const float s = sqrt((1 + cosTheta) * 2);
	const float invs = 1 / s;

	return glm::quat(
		s * 0.5f,
		rotationAxis.x * invs,
		rotationAxis.y * invs,
		rotationAxis.z * invs
	);
}

glm::quat Engine::Math::CalculateRotationBetweenOrientations(glm::quat start, glm::quat end)
{
	return end * inverse(start);
}

// Stolen from https://stackoverflow.com/questions/44705398/about-glm-quaternion-rotation
glm::vec3 Engine::Math::RotateVector(const glm::vec3& v, const glm::quat& q)
{
	const glm::vec3 quatAsVector = { q.x, q.y, q.z };
	return v * (q.w * q.w - dot(quatAsVector, quatAsVector)) + 2.0f * quatAsVector * dot(quatAsVector, v) + 2.0f * q.w * cross(quatAsVector, v);
}