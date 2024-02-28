#include "Precomp.h"
#include "Components/Pathfinding/Geometry2d.hpp"

#include <predicates/predicates.h>

#include <glm/gtx/norm.hpp>

using namespace geometry2d;
using namespace glm;

bool geometry2d::IsPointLeftOfLine(const vec2& point, const vec2& line1, const vec2& line2)
{
	double p[2] = {point.x, point.y};
	double la[2] = {line1.x, line1.y};
	double lb[2] = {line2.x, line2.y};
	return RobustPredicates::orient2d(p, la, lb) > 0;
}

bool geometry2d::IsPointRightOfLine(const vec2& point, const vec2& line1, const vec2& line2)
{
	double p[2] = {point.x, point.y};
	double la[2] = {line1.x, line1.y};
	double lb[2] = {line2.x, line2.y};
	return RobustPredicates::orient2d(p, la, lb) < 0;
}

bool geometry2d::IsClockwise(const Polygon& polygon)
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

bool geometry2d::IsPointInsidePolygon(const vec2& point, const Polygon& polygon)
{
	// Adapted from: https://wrfranklin.org/Research/Short_Notes/pnpoly.html

	size_t i, j;
	size_t n = polygon.size();
	bool inside = false;

	for (i = 0, j = n - 1; i < n; j = i++)
	{
		if ((polygon[i].y > point.y != polygon[j].y > point.y) &&
			(point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) +
				polygon[i].x))
			inside = !inside;
	}

	return inside;
}

vec2 geometry2d::GetNearestPointOnLineSegment(const vec2& p, const vec2& segmentA, const vec2& segmentB)
{
	float t = dot(p - segmentA, segmentB - segmentA) / distance2(segmentA, segmentB);
	if (t <= 0) return segmentA;
	if (t >= 1) return segmentB;
	return (1 - t) * segmentA + t * segmentB;
}

vec2 geometry2d::ComputeCenterOfPolygon(const Polygon& polygon)
{
	vec2 total(0, 0);
	for (const vec2& p : polygon) total += p;
	return total / static_cast<float>(polygon.size());
}
