#include "Precomp.h"
#include "Components/Pathfinding/Geometry2d.hpp"

#include <predicates/predicates.h>

#include <glm/gtx/norm.hpp>

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

bool Engine::IsClockwise(const Polygon& polygon)
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

bool Engine::IsPointInsidePolygon(glm::vec2 point, const Polygon& polygon)
{
	// Adapted from: https://wrfranklin.org/Research/Short_Notes/pnpoly.html

	size_t i, j;
	const size_t n = polygon.size();
	bool inside = false;

	for (i = 0, j = n - 1; i < n; j = i++)
	{
		if ((polygon[i].y > point.y != polygon[j].y > point.y) &&
			(point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
			inside = !inside;
	}

	return inside;
}

bool Engine::IsPointInsideDisk(glm::vec2 point, glm::vec2 diskCentre, float diskRadius)
{
	return glm::distance2(point, diskCentre) < diskRadius * diskRadius;
}

glm::vec2 Engine::GetNearestPointOnLineSegment(glm::vec2 p, glm::vec2 segmentA, glm::vec2 segmentB)
{
	const float t = dot(p - segmentA, segmentB - segmentA) / distance2(segmentA, segmentB);
	if (t <= 0) return segmentA;
	if (t >= 1) return segmentB;
	return (1 - t) * segmentA + t * segmentB;
}

glm::vec2 Engine::ComputeCenterOfPolygon(const Polygon& polygon)
{
	glm::vec2 total(0, 0);
	for (glm::vec2 p : polygon) total += p;
	return total / static_cast<float>(polygon.size());
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
