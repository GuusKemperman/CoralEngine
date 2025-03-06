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
		LOG(LogUnitTest, Error, "*Not overlapping* test failed.");
		return UnitTest::Failure;
	}
	// overlapping
	if (!Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 2.f, { 3.f, 0.f }, 2.f))
	{
		LOG(LogUnitTest, Error, "*Overlapping* test failed.");
		return UnitTest::Failure;
	}
	// encapsulated
	if (!Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 1.f, { 0.f, 0.f }, 2.f))
	{
		LOG(LogUnitTest, Error, "*Encapsulated* test failed.");
		return UnitTest::Failure;
	}
	// same circle
	if (!Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 1.f, { 0.f, 0.f }, 1.f))
	{
		LOG(LogUnitTest, Error, "*Same circle* test failed.");
		return UnitTest::Failure;
	}
	// one point collision
	if (!Physics2DUnitTestAccess::CollisionCheckDiskDiskUnitTest({ 0.f, 0.f }, 1.f, { 2.f, 0.f }, 1.f))
	{
		LOG(LogUnitTest, Error, "*One point collision* test failed.");
		return UnitTest::Failure;
	}
	return UnitTest::Success;
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
		LOG(LogUnitTest, Error, "*Not overlapping* test failed.");
		return UnitTest::Failure;
	}
	// overlapping 1 edge
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 1.f, { 1.5f, 0.f }, polygonPoints))
	{
		LOG(LogUnitTest, Error, "*Overlapping 1 edge* test failed.");
		return UnitTest::Failure;
	}
	// overlapping more edges
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 2.f, { 2.f, 0.f }, polygonPoints))
	{
		LOG(LogUnitTest, Error, "*Overlapping more edges* test failed.");
		return UnitTest::Failure;
	}
	// polygon in circle
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 2.f, { 0.f, 0.f }, polygonPoints))
	{
		LOG(LogUnitTest, Error, "*Polygon in circle* test failed.");
		return UnitTest::Failure;
	}
	// circle in polygon
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 0.5f, { 0.f, 0.f }, polygonPoints))
	{
		LOG(LogUnitTest, Error, "*Circle in polygon* test failed.");
		return UnitTest::Failure;
	}
	// one point edge collision
	if (!Physics2DUnitTestAccess::CollisionCheckDiskPolygonUnitTest({ 0.f, 0.f }, 1.f, { 2.f, 0.f }, polygonPoints))
	{
		LOG(LogUnitTest, Error, "*One point edge collision* test failed.");
		return UnitTest::Failure;
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
		LOG(LogUnitTest, Error, "*One point corner collision* test failed.");
		return UnitTest::Failure;
	}
	return UnitTest::Success;
}
