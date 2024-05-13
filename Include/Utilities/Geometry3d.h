#pragma once


namespace CE
{
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

	struct AABB3D
	{
		AABB3D() = default;
		AABB3D(Span<const glm::vec3> points);

		glm::vec3 mMin{ std::numeric_limits<float>::infinity() };
		glm::vec3 mMax{ -std::numeric_limits<float>::infinity() };
	};

	inline AABB3D::AABB3D(Span<const glm::vec3> points)
	{
		for (const glm::vec3& point : points)
		{
			mMin = glm::min(point, mMin);
			mMax = glm::max(point, mMax);
		}
	}

	inline float TimeOfRayIntersection(Ray3D& ray, const Triangle3D& tri)
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

	inline bool AreOverlapping(const Ray3D& ray, const AABB3D& aabb)
	{
		float tx1 = (aabb.mMin.x - ray.mOrigin.x) / ray.mDirection.x, tx2 = (aabb.mMax.x - ray.mOrigin.x) / ray.mDirection.x;
		float tmin = glm::min(tx1, tx2), tmax = glm::max(tx1, tx2);
		float ty1 = (aabb.mMin.y - ray.mOrigin.y) / ray.mDirection.y, ty2 = (aabb.mMax.y - ray.mOrigin.y) / ray.mDirection.y;
		tmin = glm::max(tmin, glm::min(ty1, ty2)), tmax = glm::min(tmax, glm::max(ty1, ty2));
		float tz1 = (aabb.mMin.z - ray.mOrigin.z) / ray.mDirection.z, tz2 = (aabb.mMax.z - ray.mOrigin.z) / ray.mDirection.z;
		tmin = glm::max(tmin, glm::min(tz1, tz2)), tmax = glm::min(tmax, glm::max(tz1, tz2));
		return tmax >= tmin && tmax > 0;
	}
}