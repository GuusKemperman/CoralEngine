#include "Precomp.h"

#include "Core/UnitTests.h"
#include "World/Physics.h"

namespace CE
{
	struct Physics2DUnitTestAccess
	{
		static bool CollisionCheckDiskDiskUnitTest(glm::vec2 center1, float radius1, glm::vec2 center2, float radius2)
		{
			Physics::CollisionData collision;
			return Physics::CollisionCheck(TransformedDisk{ center1, radius1 }, TransformedDisk{ center2, radius2 }, collision);
		}

		static bool CollisionCheckDiskPolygonUnitTest(glm::vec2 diskCenter, float diskRadius, glm::vec2 polygonPos, std::vector<glm::vec2> polygonPoints)
		{
			for (glm::vec2& p : polygonPoints)
			{
				p += polygonPos;
			}

			Physics::CollisionData collision;
			return Physics::CollisionCheck(TransformedDisk{ diskCenter, diskRadius }, TransformedPolygon{ std::move(polygonPoints) }, collision);
		}
	};
}

using namespace CE;

UNIT_TEST(PhysicsSystem, CollisionCheckDiskDisk)
{
	// not overlapping
	if (Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 1.f, { 5.f, 0.f }, 1.f))
	{
		TEST_FAILURE("*Not overlapping* test failed.");
	}
	// overlapping
	if (!Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 2.f, { 3.f, 0.f }, 2.f))
	{
		TEST_FAILURE("*Overlapping* test failed.");
	}
	// encapsulated
	if (!Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 1.f, { 0.f, 0.f }, 2.f))
	{
		TEST_FAILURE("*Encapsulated* test failed.");
	}
	// same circle
	if (!Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 1.f, { 0.f, 0.f }, 1.f))
	{
		TEST_FAILURE("*Same circle* test failed.");
	}
	// one point collision
	if (!Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 1.f, { 2.f, 0.f }, 1.f))
	{
		TEST_FAILURE("*One point collision* test failed.");
	}
}

UNIT_TEST(PhysicsSystem, CollisionCheckDiskPolygon)
{
	const std::vector<glm::vec2> polygonPoints = {
		{1.f, 1.f},
		{1.f, -1.f},
		{-1.f, -1.f},
		{-1.f, 1.f},
	};

	// not overlapping
	if (Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 1.f, { 5.f, 0.f }, polygonPoints))
	{
		TEST_FAILURE("*Not overlapping* test failed.");
	}
	// overlapping 1 edge
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 1.f, { 1.5f, 0.f }, polygonPoints))
	{
		TEST_FAILURE("*Overlapping 1 edge* test failed.");
	}
	// overlapping more edges
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 2.f, { 2.f, 0.f }, polygonPoints))
	{
		TEST_FAILURE("*Overlapping more edges* test failed.");
	}
	// polygon in circle
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 2.f, { 0.f, 0.f }, polygonPoints))
	{
		TEST_FAILURE("*Polygon in circle* test failed.");
	}
	// circle in polygon
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 0.5f, { 0.f, 0.f }, polygonPoints))
	{
		TEST_FAILURE("*Circle in polygon* test failed.");
	}
	// one point edge collision
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 1.f, { 2.f, 0.f }, polygonPoints))
	{
		TEST_FAILURE("*One point edge collision* test failed.");
	}
	const std::vector<glm::vec2> polygonPointsCornerTest = {
		{-1.f, 0.f},
		{0.f, 1.f},
		{1.f, 0.f},
		{0.f, -1.f},
	};
	// one point corner collision
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 1.f, { 2.f, 0.f }, polygonPointsCornerTest))
	{
		TEST_FAILURE("*One point corner collision* test failed.");
	}
}
