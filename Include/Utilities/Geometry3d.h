#pragma once


namespace CE
{
	struct Line3D
	{
		glm::vec3 mStart{};
		glm::vec3 mEnd{};
	};

	struct Triangle3D
	{
		glm::vec3 mVertex0{};
		glm::vec3 mVertex1{};
		glm::vec3 mVertex2{};
	};

	struct Ray3D
	{
		glm::vec3 mOrigin{};
		glm::vec3 mDirection{};
	};

	float RayTriangleIntersection(Ray3D& ray, const Triangle3D& tri)
	{
		const glm::vec3 edge1 = tri.mVertex1 - tri.mVertex0;
		const glm::vec3 edge2 = tri.mVertex2 - tri.mVertex0;
		const glm::vec3 h = glm::cross(ray.mDirection, edge2);
		const float a = glm::dot(edge1, h);
		if (a > -0.0001f && a < 0.0001f)
		{
			return std::numeric_limits<float>::infinity();
		}

		const float f = 1 / a;
		const glm::vec3 s = ray.mOrigin - tri.mVertex0;
		const float u = f * glm::dot(s, h);
		if (u < 0 || u > 1)
		{
			return std::numeric_limits<float>::infinity();
		}

		const glm::vec3 q = glm::cross(s, edge1);
		const float v = f * glm::dot(ray.mDirection, q);
		if (v < 0 || u + v > 1)
		{
			return std::numeric_limits<float>::infinity();
		}

		const float t = f * glm::dot(edge2, q);
		if (t > 0.0001f)
		{
			return t;
		}
		return std::numeric_limits<float>::infinity();
	}
}