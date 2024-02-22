#pragma once

#include <vector>

#include "glm/vec2.hpp"

/// <summary>
/// A namespace containing functions related to 2D geometry.
/// </summary>
namespace geometry2d
{
	// Input data types
	using Polygon = std::vector<glm::vec2>;
	using PolygonList = std::vector<Polygon>;

	/// <summary>
	/// Simple struct representing an axis-aligned bounding box.
	/// </summary>
	struct AABB
	{
	private:
		glm::vec2 m_min;
		glm::vec2 m_max;

	public:
		AABB(const glm::vec2& minPos, const glm::vec2& maxPos) : m_min(minPos), m_max(maxPos)
		{
		}

		/// <summary>
		/// Computes and returns the four boundary vertices of this AABB.
		/// </summary>
		Polygon ComputeBoundary() const
		{
			return {m_min, glm::vec2(m_max.x, m_min.y), m_max, glm::vec2(m_min.x, m_max.y)};
		}

		/// <summary>
		/// Computes and returns the center coordinate of this AABB.
		/// </summary>
		glm::vec2 ComputeCenter() const { return (m_min + m_max) / 2.f; }

		/// <summary>
		/// Computes and returns the size of this AABB, wrapped in a vec2 (x=width, y=height).
		/// </summary>
		/// <returns></returns>
		glm::vec2 ComputeSize() const { return m_max - m_min; }

		const glm::vec2& GetMin() const { return m_min; }
		const glm::vec2& GetMax() const { return m_max; }
	};

	/// <summary>
	/// Checks and returns whether a 2D point lies strictly to the left of an infinite directed line.
	/// </summary>
	/// <param name="point">A query point.</param>
	/// <param name="line1">A first point on the query line.</param>
	/// <param name="line2">A second point on the query line.</param>
	/// <returns>true if "point" lies strictly to the left of the infinite directed line through line1 and line2;
	/// false otherwise (i.e. if the point lies on or to the right of the line).</return>
	bool IsPointLeftOfLine(const glm::vec2& point, const glm::vec2& line1, const glm::vec2& line2);

	/// <summary>
	/// Checks and returns whether a 2D point lies strictly to the right of an infinite directed line.
	/// </summary>
	/// <param name="point">A query point.</param>
	/// <param name="line1">A first point on the query line.</param>
	/// <param name="line2">A second point on the query line.</param>
	/// <returns>true if "point" lies strictly to the right of the infinite directed line through line1 and line2;
	/// false otherwise (i.e. if the point lies on or to the left of the line).</return>
	bool IsPointRightOfLine(const glm::vec2& point, const glm::vec2& line1, const glm::vec2& line2);

	/// <summary>
	/// Checks and returns whether the points of a simple 2D polygon are given in clockwise order.
	/// </summary>
	/// <param name="polygon">A list of 2D points describing the boundary of a simple polygon (i.e. at least 3 points, nonzero area,
	/// not self-intersecting).</param> <returns>true if the points are given in clockwise order, false otherwise.</returns>
	bool IsClockwise(const Polygon& polygon);

	/// <summary>
	/// Checks and returns whether a given point lies inside a given 2D polygon.
	/// </summary>
	/// <param name="point">A query point.</param>
	/// <param name="polygon">A simple 2D polygon.</param>
	/// <returns>true if the point lies inside the polygon, false otherwise.</return>
	bool IsPointInsidePolygon(const glm::vec2& point, const Polygon& polygon);

	/// <summary>
	/// Computes and returns the centroid of a given polygon (= the average of its boundary points).
	/// </summary>
	glm::vec2 ComputeCenterOfPolygon(const Polygon& polygon);

	/// <summary>
	/// Computes and returns the nearest point on a line segment segmentA-segmentB to another point p.
	/// </summary>
	/// <param name="p">A query point.</param>
	/// <param name="segmentA">The first endpoint of a line segment.</param>
	/// <param name="segmentB">The second endpoint of a line segment.</param>
	/// <returns>The point on the line segment segmentA-segmentB that is closest to p.</returns>
	glm::vec2 GetNearestPointOnLineSegment(const glm::vec2& p, const glm::vec2& segmentA, const glm::vec2& segmentB);
}; // namespace bee::geometry2d
