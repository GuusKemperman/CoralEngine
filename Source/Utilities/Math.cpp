#include "Precomp.h"
#include "Utilities/Math.h"

//glm::vec2 CE::Math::GLFWPixelToClipSpace(const glm::ivec2& pixelPosition)
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
glm::quat CE::Math::CalculateOrientationTowards(glm::quat current, const glm::quat& target, const float maxAngle)
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

glm::quat CE::Math::CalculateRotationBetweenOrientations(glm::vec3 start, glm::vec3 dest)
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

glm::quat CE::Math::CalculateRotationBetweenOrientations(glm::quat start, glm::quat end)
{
	return end * inverse(start);
}

// Stolen from https://stackoverflow.com/questions/44705398/about-glm-quaternion-rotation
glm::vec3 CE::Math::RotateVector(const glm::vec3& v, const glm::quat& q)
{
	const glm::vec3 quatAsVector = { q.x, q.y, q.z };
	return v * (q.w * q.w - dot(quatAsVector, quatAsVector)) + 2.0f * quatAsVector * dot(quatAsVector, v) + 2.0f * q.w * cross(quatAsVector, v);
}

std::optional<std::vector<glm::vec3>> CE::Math::CalculateTangents(
	const void* const indices,
	const size_t numOfIndices,
	const bool areIndices16Bit,
	const glm::vec3* const positions,
	const glm::vec3* const normals,
	const glm::vec2* const texCoords,
	const size_t numOfVertices)
{
	if (indices == nullptr
		|| positions == nullptr
		|| normals == nullptr
		|| texCoords == nullptr)
	{
		return std::nullopt;
	}

	std::vector<glm::vec3> tangents(numOfVertices, glm::vec3(0.0f));

	// Loop through each triangle
	for (size_t i = 0; i < numOfIndices; i += 3) {
		// Get vertex indices of the triangle
		int32 i1 = areIndices16Bit ? static_cast<int>(static_cast<const uint16*>(indices)[i + 0]) : static_cast<const int32*>(indices)[i];
		int32 i2 = areIndices16Bit ? static_cast<int>(static_cast<const uint16*>(indices)[i + 1]) : static_cast<const int32*>(indices)[i + 1];
		int32 i3 = areIndices16Bit ? static_cast<int>(static_cast<const uint16*>(indices)[i + 2]) : static_cast<const int32*>(indices)[i + 2];

		// Calculate triangle edges
		glm::vec3 edge1 = positions[i2] - positions[i1];
		glm::vec3 edge2 = positions[i3] - positions[i1];

		// UV deltas
		glm::vec2 deltaUV1 = texCoords[i2] - texCoords[i1];
		glm::vec2 deltaUV2 = texCoords[i3] - texCoords[i1];

		// Calculate tangent
		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
		glm::vec3 tangent{};
		tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
		tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
		tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

		// Add tangent to the vertices' tangent
		tangents[i1] += tangent;
		tangents[i2] += tangent;
		tangents[i3] += tangent;
	}

	// Normalize tangents
	for (size_t i = 0; i < numOfVertices; ++i) 
	{
		tangents[i] = glm::normalize(tangents[i]);
	}

	return tangents;
}