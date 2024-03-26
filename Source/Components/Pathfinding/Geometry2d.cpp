#include "Precomp.h"
#include "Components/Pathfinding/Geometry2d.h"

#include <predicates/predicates.h>

#include <glm/gtx/norm.hpp>

float Engine::TransformedAABB::SignedDistance(glm::vec2 toPoint) const
{
	const glm::vec2 halfExtends = (mMax - mMin) * 0.5f;
	const glm::vec2 boxCentre = mMin + halfExtends;

	glm::vec2 d = abs(boxCentre - toPoint) - halfExtends;
	return glm::length(glm::max(d, 0.0f)) + glm::min(glm::max(d.x, d.y), 0.0f);
}

Engine::TransformedPolygon Engine::TransformedAABB::GetAsPolygon() const
{
	return {
	{
		mMin,
		glm::vec2{ mMax.x, mMin.y },
		mMax,
		glm::vec2{ mMin.x, mMax.y }
	},
	*this
	};
}

float Engine::TransformedDisk::SignedDistance(const glm::vec2 toPoint) const
{
	const float dist = glm::distance(mCentre, toPoint);
	return dist - mRadius;
}

Engine::TransformedPolygon Engine::TransformedDisk::GetAsPolygon() const
{
	constexpr float dt = glm::two_pi<float>() / 16.0f;

	PolygonPoints points{};

	for (float t = 0.0f; t < glm::two_pi<float>() - dt; t += dt)
	{
		points.emplace_back(mCentre.x + mRadius * cos(t + dt), mCentre.y + mRadius * sin(t + dt));
	}

	return TransformedPolygon{ std::move(points), { mCentre - glm::vec2{mRadius}, mCentre + glm::vec2{mRadius} } };
}


// https://iquilezles.org/articles/distfunctions2d/
float Engine::TransformedPolygon::SignedDistance(const glm::vec2 point) const
{
	float d = glm::distance2(point, mPoints[0]);
	float s = 1.0f;

	for (uint32 i = 0, j = static_cast<uint32>(mPoints.size()) - 1u; i < mPoints.size(); j = i++)
	{
		glm::vec2 e = mPoints[j] - mPoints[i];
		glm::vec2 w = point - mPoints[i];
		glm::vec2 b = w - e * glm::clamp(glm::dot(w, e) / glm::length2(e), 0.0f, 1.0f);
		d = glm::min(d, glm::length2(b));
		glm::bvec3 c = { point.y >= mPoints[i].y, point.y < mPoints[j].y, e.x * w.y > e.y * w.x };
		if (glm::all(c) || glm::all(glm::not_(c)))
		{
			s *= -1.0f;
		}
	}

	return s * sqrtf(d);
}

glm::vec2 Engine::TransformedPolygon::GetCentre() const
{
	if (mPoints.empty())
	{
		return glm::vec2{};
	}

	glm::vec2 total(0, 0);
	for (glm::vec2 p : mPoints) total += p;
	return total / static_cast<float>(mPoints.size());
}

Engine::TransformedPolygon::TransformedPolygon(PolygonPoints&& transformedPoints):
	mPoints(std::move(transformedPoints))
{
	for (const glm::vec2 point : mPoints)
	{
		mBoundingBox.mMin.x = glm::min(mBoundingBox.mMin.x, point.x);
		mBoundingBox.mMin.y = glm::min(mBoundingBox.mMin.y, point.y);
		mBoundingBox.mMax.x = glm::max(mBoundingBox.mMax.x, point.x);
		mBoundingBox.mMax.y = glm::max(mBoundingBox.mMax.y, point.y);
	}
}

Engine::TransformedPolygon::TransformedPolygon(PolygonPoints&& transformedPoints, TransformedAABB boundingBox) :
	mPoints(std::move(transformedPoints)),
	mBoundingBox(boundingBox)
{
#ifdef LOGGING_ENABLED
	for (const glm::vec2 point : mPoints)
	{
		if (!AreOverlapping(boundingBox, point))
		{
			LOG(LogCore, Error, "Invalid bounding box provided: {}, {} was not in boxMin {}, {} boxMax {}, {}", 
				point.x, point.y,
				boundingBox.mMin.x, boundingBox.mMin.y,
				boundingBox.mMax.x, boundingBox.mMax.y);
		}
	}
#endif
}

bool Engine::IsPointLeftOfLine(glm::vec2 point, glm::vec2 line1, glm::vec2 line2)
{
	double p[2] = {point.x, point.y};
	double la[2] = {line1.x, line1.y};
	double lb[2] = {line2.x, line2.y};
	return RobustPredicates::orient2d(p, la, lb) > 0;
}

bool Engine::IsPointRightOfLine(glm::vec2 point, glm::vec2 line1, glm::vec2 line2)
{
	double p[2] = {point.x, point.y};
	double la[2] = {line1.x, line1.y};
	double lb[2] = {line2.x, line2.y};
	return RobustPredicates::orient2d(p, la, lb) < 0;
}

bool Engine::IsClockwise(const PolygonPoints& polygon)
{
	size_t n = polygon.size();
	assert(n > 2);
	float signedArea = 0.f;

	for (size_t i = 0; i < n; ++i)
	{
		const auto& p0 = polygon[i];
		const auto& p1 = polygon[(i + 1) % n];

		signedArea += (p0.x * p1.y - p1.x * p0.y);
	}

	// Technically we now have 2 * the signed area.
	// But for the "is clockwise" check, we only care about the sign of this number,
	// so there is no need to divide by 2.
	return signedArea < 0.f;
}

glm::vec2 Engine::GetNearestPointOnLineSegment(glm::vec2 p, glm::vec2 segmentA, glm::vec2 segmentB)
{
	const float t = dot(p - segmentA, segmentB - segmentA) / distance2(segmentA, segmentB);
	if (t <= 0) return segmentA;
	if (t >= 1) return segmentB;
	return (1 - t) * segmentA + t * segmentB;
}

glm::vec2 Engine::GetNearestPointOnPolygonBoundary(glm::vec2 point, const std::vector<glm::vec2>& polygon)
{
	float bestDist = std::numeric_limits<float>::max();
	glm::vec2 bestNearest(0.f, 0.f);

	const size_t n = polygon.size();
	for (size_t i = 0; i < n; ++i)
	{
		glm::vec2 nearest = GetNearestPointOnLineSegment(point, polygon[i], polygon[(i + 1) % n]);
		const float dist = distance2(point, nearest);
		if (dist < bestDist)
		{
			bestDist = dist;
			bestNearest = nearest;
		}
	}

	return bestNearest;
}

bool Engine::AreOverlapping(const TransformedDisk diskA, const TransformedDisk diskB)
{
	const float combinedRadius = diskA.mRadius + diskB.mRadius;
	const float combinedRadius2 = combinedRadius * combinedRadius;;
	return glm::distance2(diskA.mCentre, diskB.mCentre) <= combinedRadius2;
}

bool Engine::AreOverlapping(const TransformedAABB boxA, const TransformedAABB boxB)
{
	return boxA.mMin.x < boxB.mMax.x
		&& boxA.mMax.x > boxB.mMin.x
		&& boxA.mMax.y > boxB.mMin.y
		&& boxA.mMin.y < boxB.mMax.y;
}

bool Engine::AreOverlapping(const TransformedPolygon& polygonA, const TransformedPolygon& polygonB)
{
	if (!AreOverlapping(polygonA.mBoundingBox, polygonB.mBoundingBox))
	{
		return false;
	}

	for (size_t i = 0, j = polygonA.mPoints.size() - 1; i < polygonA.mPoints.size(); j = i++)
	{
		if (AreOverlapping(polygonB, Line{ polygonA.mPoints[i], polygonA.mPoints[j] }))
		{
			return true;
		}
	}

	// None of the lines intersect, but it's still possible that one of them is completely inside the other one.
	return (!polygonA.mPoints.empty() && !polygonB.mPoints.empty())
		&& (AreOverlapping(polygonA, polygonB.mPoints.front())
			|| AreOverlapping(polygonB, polygonA.mPoints.front()));
}

bool Engine::AreOverlapping(const TransformedPolygon& polygon, const TransformedDisk disk)
{
	if (!AreOverlapping(polygon.mBoundingBox, disk))
	{
		return false;
	}

	for (size_t i = 0, j = polygon.mPoints.size() - 1; i < polygon.mPoints.size(); j = i++)
	{
		if (AreOverlapping(disk, Line{ polygon.mPoints[i], polygon.mPoints[j] }))
		{
			return true;
		}
	}

	// None of the lines intersect and none of the polygon's points are inside the disk, but maybe the disk is completely inside the polygon
	return AreOverlapping(polygon, disk.mCentre);
}

bool Engine::AreOverlapping(const TransformedPolygon& polygon, const TransformedAABB aabb)
{
	if (!AreOverlapping(polygon.mBoundingBox, aabb))
	{
		return false;
	}

	for (size_t i = 0, j = polygon.mPoints.size() - 1; i < polygon.mPoints.size(); j = i++)
	{
		if (AreOverlapping(aabb, Line{ polygon.mPoints[i], polygon.mPoints[j] }))
		{
			return true;
		}
	}

	// None of the lines of the polygon intersected with the box. Either the box is fully outside, or fully inside.
	return AreOverlapping(polygon, aabb.GetCentre());
}

bool Engine::AreOverlapping(const TransformedAABB aabb, const TransformedDisk disk)
{
	// https://learnopengl.com/In-Practice/2D-Game/Collisions/Collision-Detection
	// calculate AABB info (center, half-extents)
	const glm::vec2 size = aabb.GetSize();
	const glm::vec2 halfSize = size * 0.5f;
	const glm::vec2 aabbCentre = aabb.mMin + halfSize;

	// get difference vector between both centers
	glm::vec2 difference = disk.mCentre - aabbCentre;
	glm::vec2 clamped = glm::clamp(difference, -halfSize, halfSize);
	// add clamped value to AABB_center and we get the value of box closest to disk
	glm::vec2 closest = aabbCentre + clamped;
	// retrieve vector between center disk and closest point AABB and check if length <= radius
	difference = closest - disk.mCentre;
	return glm::length(difference) < disk.mRadius;
}

// Function needed for line-line intersection
static bool onSegment(glm::vec2 p, glm::vec2 q, glm::vec2 r)
{
	return q.x <= (glm::max)(p.x, r.x) && q.x >= (glm::min)(p.x, r.x) &&
		q.y <= (glm::max)(p.y, r.y) && q.y >= (glm::min)(p.y, r.y);
}
// Function needed for line-line intersection
static int orientation(glm::vec2 p, glm::vec2 q, glm::vec2 r)
{
	const float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
	return val == 0 ? 0 : ((val > 0) ? 1 : 2);
}

bool Engine::AreOverlapping(const Line line1, const Line line2)
{
	// https://www.geeksforgeeks.org/check-if-two-given-line-segments-intersect/

	// Find the four orientations needed for general and
	// special cases
	const int o1 = orientation(line1.mStart, line1.mEnd, line2.mStart);
	const int o2 = orientation(line1.mStart, line1.mEnd, line2.mEnd);
	const int o3 = orientation(line2.mStart, line2.mEnd, line1.mStart);
	const int o4 = orientation(line2.mStart, line2.mEnd, line1.mEnd);

	// General case
	return (o1 != o2 && o3 != o4)
		|| (o1 == 0 && onSegment(line1.mStart, line2.mStart, line1.mEnd))
		|| (o2 == 0 && onSegment(line1.mStart, line2.mEnd, line1.mEnd))
		|| (o3 == 0 && onSegment(line2.mStart, line1.mStart, line2.mEnd))
		|| (o4 == 0 && onSegment(line2.mStart, line1.mEnd, line2.mEnd));
}

bool Engine::AreOverlapping(const Line line, const TransformedAABB aabb)
{
	// https://tavianator.com/2022/ray_box_boundary.html
	// While this is fast, rays completely inside the box will be considered an intersection, which may not be what we want.
	float tmin = 0.0, tmax = INFINITY;

	const glm::vec2 invRayDir = 1.0f / (line.mEnd - line.mStart);

	for (int i = 0; i < 2; ++i)
	{
		float t1 = (aabb.mMin[i] - line.mStart[i]) * invRayDir[i];
		float t2 = (aabb.mMax[i] - line.mStart[i]) * invRayDir[i];

		tmin = glm::max(tmin, glm::min(glm::min(t1, t2), tmax));
		tmax = glm::min(tmax, glm::max(glm::max(t1, t2), tmin));
	}

	return tmin < tmax && tmin <= 1.0f;
}

bool Engine::AreOverlapping(const Line line, const TransformedDisk disk)
{
	// Uses the signed distance to a line segment function from:
	// https://iquilezles.org/articles/distfunctions2d/

	const glm::vec2 pa = disk.mCentre - line.mStart, ba = line.mEnd - line.mStart;

	float h = glm::clamp(dot(pa, ba) / dot(ba, ba), 0.0f, 1.0f);
	float distance2ToLine = length2(pa - ba * h);
	return distance2ToLine <= disk.mRadius * disk.mRadius;
}

bool Engine::AreOverlapping(const Line line, const TransformedPolygon& polygon)
{
	if (!AreOverlapping(polygon.mBoundingBox, line)
		&& !AreOverlapping(polygon.mBoundingBox, line.mStart)) // The ray might have started and ended inside the box, hence the additonal check
	{
		return false;
	}

	for (size_t i = 0, j = polygon.mPoints.size() - 1; i < polygon.mPoints.size(); j = i++)
	{
		if (AreOverlapping(line, Line{ polygon.mPoints[i], polygon.mPoints[j] }))
		{
			return true;
		}
	}

	return false;
}

bool Engine::AreOverlapping(const TransformedDisk disk, const TransformedPolygon& polygon)
{
	return AreOverlapping(polygon, disk);
}

bool Engine::AreOverlapping(const TransformedAABB box, const TransformedPolygon& polygon)
{
	return AreOverlapping(polygon, box);
}

bool Engine::AreOverlapping(const TransformedDisk disk, const TransformedAABB box)
{
	return AreOverlapping(box, disk);
}

bool Engine::AreOverlapping(const TransformedAABB aabb, const Line line)
{
	return AreOverlapping(line, aabb);
}

bool Engine::AreOverlapping(const TransformedDisk disk, const Line line)
{
	return AreOverlapping(line, disk);
}

bool Engine::AreOverlapping(const TransformedPolygon& polygon, const Line line)
{
	return AreOverlapping(line, polygon);
}

bool Engine::AreOverlapping(TransformedDisk disk, glm::vec2 point)
{
	return glm::distance2(point, disk.mCentre) < disk.mRadius * disk.mRadius;
}

bool Engine::AreOverlapping(TransformedAABB aabb, glm::vec2 point)
{
	return point.x >= aabb.mMin.x
		&& point.x <= aabb.mMax.x
		&& point.y >= aabb.mMin.y
		&& point.y <= aabb.mMax.y;
}

bool Engine::AreOverlapping(const TransformedPolygon& polygon, glm::vec2 point)
{
	if (!AreOverlapping(polygon.mBoundingBox, point))
	{
		return false;
	}

	bool contains{};
	for (size_t i = 0, j = polygon.mPoints.size() - 1; i < polygon.mPoints.size(); j = i++)
	{
		const glm::vec2 iVert = polygon.mPoints[i];
		const glm::vec2 jVert = polygon.mPoints[j];

		if ((iVert.y > point.y) != (jVert.y > point.y)
			&& (point.x < (jVert.x - iVert.x) * (point.y - iVert.y) / (jVert.y - iVert.y) + iVert.x))
		{
			contains = !contains;
		}
	}

	return contains;
}

bool Engine::AreOverlapping(glm::vec2 point, TransformedDisk disk)
{
	return AreOverlapping(disk, point);
}

bool Engine::AreOverlapping(glm::vec2 point, TransformedAABB aabb)
{
	return AreOverlapping(aabb, point);
}

bool Engine::AreOverlapping(glm::vec2 point, const TransformedPolygon& polygon)
{
	return AreOverlapping(polygon, point);
}

float Engine::TimeOfLineIntersection(Line line1, Line line2)
{
	float s1_x, s1_y, s2_x, s2_y;
	s1_x = line1.mEnd.x - line1.mStart.x;     s1_y = line1.mEnd.y - line1.mStart.y;
	s2_x = line2.mEnd.x - line2.mStart.x;     s2_y = line2.mEnd.y - line2.mStart.y;

	float s, t;
	s = (-s1_y * (line1.mStart.x - line2.mStart.x) + s1_x * (line1.mStart.y - line2.mStart.y)) / (-s2_x * s1_y + s1_x * s2_y);
	t = (s2_x * (line1.mStart.y - line2.mStart.y) - s2_y * (line1.mStart.x - line2.mStart.x)) / (-s2_x * s1_y + s1_x * s2_y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		return t;
	}
	return INFINITY;
}

float Engine::TimeOfLineIntersection(Line line, TransformedAABB aabb)
{
	float tmin = 0.0, tmax = INFINITY;

	glm::vec2 invRayDir = 1.0f / (line.mEnd - line.mStart);

	for (int i = 0; i < 2; ++i)
	{
		float t1 = (aabb.mMin[i] - line.mStart[i]) * invRayDir[i];
		float t2 = (aabb.mMax[i] - line.mStart[i]) * invRayDir[i];

		tmin = glm::max(tmin, glm::min(glm::min(t1, t2), tmax));
		tmax = glm::min(tmax, glm::max(glm::max(t1, t2), tmin));
	}

	return tmin < tmax ? tmin : INFINITY;
}

float Engine::TimeOfLineIntersection(Line line, TransformedDisk disk)
{
	glm::vec2 f = line.mStart - disk.mCentre;
	glm::vec2 d = line.mEnd - line.mStart;

	float a = glm::dot(d, d);
	float b = 2 * glm::dot(f, d);
	float c = glm::dot(f, f) - (disk.mRadius * disk.mRadius);

	float discriminant = b * b - 4 * a * c;
	if (discriminant < 0)
	{
		// no intersection
		return INFINITY;
	}

	// ray didn't totally miss sphere,
	// so there is a solution to
	// the equation.

	discriminant = sqrt(discriminant);

	// either solution may be on or off the ray so need to test both
	// t1 is always the smaller value, because BOTH discriminant and
	// a are nonnegative.
	float t1 = (-b - discriminant) / (2 * a);

	// 3x HIT cases:
	//          -o->             --|-->  |            |  --|->
	// Impale(t1 hit,t2 hit), Poke(t1 hit,t2>1), ExitWound(t1<0, t2 hit), 

	// 3x MISS cases:
	//       ->  o                     o ->              | -> |
	// FallShort (t1>1,t2>1), Past (t1<0,t2<0), CompletelyInside(t1<0, t2>1)

	if (t1 >= 0 && t1 <= 1)
	{
		// t1 is the intersection, and it's closer than t2
		// (since t1 uses -b - discriminant)
		// Impale, Poke
		return t1;
	}

	float t2 = (-b + discriminant) / (2 * a);

	// here t1 didn't intersect so we are either started
	// inside the sphere or completely past it
	if ((t2 >= 0 && t2 <= 1)
		|| (t1 < 0 && t2 > 1))// Completely inside)
	{
		// ExitWound
		return t2;
	}

	// no intn: FallShort, Past
	return INFINITY;
}

float Engine::TimeOfLineIntersection(Line line, const TransformedPolygon& polygon)
{
	if (!AreOverlapping(polygon.mBoundingBox, line)
		&& !AreOverlapping(polygon.mBoundingBox, line.mStart)) // The ray might have started and ended inside the box, hence the additonal check
	{
		return INFINITY;
	}

	float earliestHitTime = INFINITY;
	for (size_t i = 0, j = polygon.mPoints.size() - 1; i < polygon.mPoints.size(); j = i++)
	{
		const float hitTime = TimeOfLineIntersection(line, Line{ polygon.mPoints[i], polygon.mPoints[j] });

		if (hitTime >= 0.0f
			&& hitTime <= 1.0f
			&& hitTime < earliestHitTime)
		{
			earliestHitTime = hitTime;
		}
	}

	return earliestHitTime;

}
