#pragma once

namespace Engine
{
	using PolygonPoints = std::vector<glm::vec2>;
	struct TransformedPolygon;
	struct TransformedDisk;

	struct Line
	{
		glm::vec2 mStart{};
		glm::vec2 mEnd{};
	};

	struct TransformedAABB
	{
		float SignedDistance(glm::vec2 toPoint) const;

		TransformedPolygon GetAsPolygon() const;
		TransformedAABB GetBoundingBox() const { return *this; }

		void CombineWith(TransformedAABB aabb)
		{
			mMin = glm::min(mMin, aabb.mMin);
			mMax = glm::max(mMax, aabb.mMax);
		}

		glm::vec2 GetCentre() const { return mMin + GetSize() * 0.5f; }
		glm::vec2 GetSize() const { return mMax - mMin; }
		float GetPerimeter() const { const glm::vec2 size = GetSize(); return size.x + size.x + size.y + size.y; }

		glm::vec2 mMin{};
		glm::vec2 mMax{};
	};

	struct TransformedDisk
	{
		float SignedDistance(glm::vec2 toPoint) const;

		TransformedPolygon GetAsPolygon() const;
		TransformedAABB GetBoundingBox() const { return { glm::vec2{mCentre.x - mRadius, mCentre.y - mRadius}, glm::vec2{mCentre.x + mRadius, mCentre.y + mRadius} }; }
		glm::vec2 GetCentre() const { return mCentre; }

		glm::vec2 mCentre{};
		float mRadius{};
	};

	struct TransformedPolygon
	{
		TransformedPolygon() = default;
		TransformedPolygon(PolygonPoints&& transformedPoints);
		TransformedPolygon(PolygonPoints&& transformedPoints, TransformedAABB boundingBox);

		float SignedDistance(glm::vec2 toPoint) const;
		const TransformedPolygon& GetAsPolygon() const { return *this; }
		const TransformedAABB& GetBoundingBox() const { return mBoundingBox; }
		glm::vec2 GetCentre() const;

		PolygonPoints mPoints{};
		TransformedAABB mBoundingBox = { glm::vec2{INFINITY, INFINITY}, glm::vec2{-INFINITY, -INFINITY} };
	};

	/// <summary>
	/// Checks and returns whether a 2D point lies strictly to the left of an infinite directed line.
	/// </summary>
	/// <param name="point">A query point.</param>
	/// <param name="line1">A first point on the query line.</param>
	/// <param name="line2">A second point on the query line.</param>
	/// <returns>true if "point" lies strictly to the left of the infinite directed line through line1 and line2;
	/// false otherwise (i.e. if the point lies on or to the right of the line).</return>
	bool IsPointLeftOfLine(glm::vec2 point, glm::vec2 line1, glm::vec2 line2);

	/// <summary>
	/// Checks and returns whether a 2D point lies strictly to the right of an infinite directed line.
	/// </summary>
	/// <param name="point">A query point.</param>
	/// <param name="line1">A first point on the query line.</param>
	/// <param name="line2">A second point on the query line.</param>
	/// <returns>true if "point" lies strictly to the right of the infinite directed line through line1 and line2;
	/// false otherwise (i.e. if the point lies on or to the left of the line).</return>
	bool IsPointRightOfLine(glm::vec2 point, glm::vec2 line1, glm::vec2 line2);

	/// <summary>
	/// Checks and returns whether the points of a simple 2D polygon are given in clockwise order.
	/// </summary>
	/// <param name="polygon">A list of 2D points describing the boundary of a simple polygon (i.e. at least 3 points, nonzero area,
	/// not self-intersecting).</param> <returns>true if the points are given in clockwise order, false otherwise.</returns>
	bool IsClockwise(const PolygonPoints& polygon);

	/// <summary>
	/// Computes and returns the nearest point on a line segment segmentA-segmentB to another point p.
	/// </summary>
	/// <param name="p">A query point.</param>
	/// <param name="segmentA">The first endpoint of a line segment.</param>
	/// <param name="segmentB">The second endpoint of a line segment.</param>
	/// <returns>The point on the line segment segmentA-segmentB that is closest to p.</returns>
	glm::vec2 GetNearestPointOnLineSegment(glm::vec2 p, glm::vec2 segmentA, glm::vec2 segmentB);

	glm::vec2 GetNearestPointOnPolygonBoundary(glm::vec2 point, const std::vector<glm::vec2>& polygon);

    bool AreOverlapping(TransformedDisk diskA, TransformedDisk diskB);

    bool AreOverlapping(TransformedAABB boxA, TransformedAABB boxB);

    bool AreOverlapping(const TransformedPolygon& polygonA, const TransformedPolygon& polygonB);

    bool AreOverlapping(const TransformedPolygon& polygon, TransformedDisk disk);

    bool AreOverlapping(const TransformedPolygon& polygon, TransformedAABB aabb);

    bool AreOverlapping(TransformedAABB aabb, TransformedDisk disk);

    bool AreOverlapping(Line line1, Line line2);

    bool AreOverlapping(Line line, TransformedAABB aabb);

    bool AreOverlapping(Line line, TransformedDisk disk);

    bool AreOverlapping(Line line, const TransformedPolygon& polygon);

    bool AreOverlapping(TransformedDisk disk, const TransformedPolygon& polygon);

    bool AreOverlapping(TransformedAABB box, const TransformedPolygon& polygon);

	bool AreOverlapping(TransformedDisk disk, TransformedAABB box);

	bool AreOverlapping(TransformedAABB aabb, Line line);

	bool AreOverlapping(TransformedDisk disk, Line line);

	bool AreOverlapping(const TransformedPolygon& polygon, Line line);

	bool AreOverlapping(TransformedDisk disk, glm::vec2 point);

	bool AreOverlapping(TransformedAABB aabb, glm::vec2 point);

	bool AreOverlapping(const TransformedPolygon& polygon, glm::vec2 point);

	bool AreOverlapping(glm::vec2 point, TransformedDisk disk);

	bool AreOverlapping(glm::vec2 point, TransformedAABB aabb);

	bool AreOverlapping(glm::vec2 point, const TransformedPolygon& polygon);

	float TimeOfLineIntersection(Line line1, Line line2);

	float TimeOfLineIntersection(Line line, TransformedAABB aabb);

	float TimeOfLineIntersection(Line line, TransformedDisk disk);

	float TimeOfLineIntersection(Line line, const TransformedPolygon& polygon);
}
