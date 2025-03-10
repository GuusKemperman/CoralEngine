#include "Precomp.h"

#include "Core/UnitTests.h"
#include "World/Physics.h"
#include "World/World.h"
#include "Components/TransformComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Utilities/Random.h"

using namespace CE;

namespace
{
	constexpr int sNumCollidersToSpawn = 300;

	CollisionRules GetRules()
	{
		CollisionRules rules{};
		rules.mLayer = CollisionLayer::Query;
		rules.SetResponse(CollisionLayer::Query, CollisionResponse::Overlap);
			return rules;
	}

	void PopulateWithRandomColliders(CE::World& world)
	{
		Registry& reg = world.GetRegistry();

		CollisionRules rules = GetRules();

		auto addLambda = [&]<typename T>(const T & collider)
		{
			for (int i = 0; i < sNumCollidersToSpawn / 3; i++)
			{
				entt::entity entity = reg.Create();
				reg.AddComponent<TransformComponent>(entity).SetWorldPosition(CE::Random::Range(glm::vec2{ -100.0f, -100.0f }, glm::vec2{ 100.0f, 100.0f }));
				reg.AddComponent<PhysicsBody2DComponent>(entity).mRules = rules;
				reg.AddComponent<T>(entity, collider);
			}
		};
		addLambda(DiskColliderComponent{ 5.0f });
		addLambda(AABBColliderComponent{ glm::vec2{ 5.0f, 5.0f } });
		addLambda(PolygonColliderComponent{ { glm::vec2{ -5.0f, 0.0f}, glm::vec2{ 0.0f, 5.0f}, glm::vec2{ 5.0f, 0.0f} } });

		world.GetPhysics().SyncWorldToPhysics();
	}

	void ShuffleColliders(CE::World& world)
	{
		for (auto [entity, transform] : world.GetRegistry().View<TransformComponent>().each())
		{
			transform.SetWorldPosition(CE::Random::Range(glm::vec2{ -100.0f, -100.0f }, glm::vec2{ 100.0f, 100.0f }));
		}
		world.GetPhysics().SyncWorldToPhysics();
	}

	UnitTest::Result ExploreSingle(CE::World& world, CE::Physics::ExploreOrder order)
	{
		for (int i = 0; i < 1000; i++)
		{
			glm::vec2 point = Random::Range(glm::vec2{ -100.0f, -100.0f }, glm::vec2{ 100.0f, 100.0f });

			entt::entity nearest = order == CE::Physics::ExploreOrder::NearestFirst ?
				world.GetPhysics().GetNearest(point, GetRules()) :
				world.GetPhysics().GetFarthest(point, GetRules());
			TEST_ASSERT(nearest != entt::null);
			float nearestDist = world.GetPhysics().GetSignedDistance(nearest, point);

			auto testLambda = [&]<typename T>()
			{
				for (auto [entity, other] : world.GetRegistry().View<T>().each())
				{
					if (entity == nearest)
					{
						continue;
					}

					float otherDist = other.SignedDistance(point);
					
					if (order == CE::Physics::ExploreOrder::NearestFirst)
					{
						TEST_ASSERT(otherDist >= nearestDist);
					}
					else
					{
						TEST_ASSERT(otherDist <= nearestDist);
					}
				}
				return UnitTest::Result::Success;
			};
			TEST_ASSERT(testLambda.operator()<TransformedDiskColliderComponent>() == UnitTest::Result::Success);
			TEST_ASSERT(testLambda.operator()<TransformedAABBColliderComponent>() == UnitTest::Result::Success);
			TEST_ASSERT(testLambda.operator()<TransformedPolygonColliderComponent>() == UnitTest::Result::Success);
		}

		return UnitTest::Result::Success;
	}

	UnitTest::Result ExploreMulti(CE::World& world, CE::Physics::ExploreOrder order)
	{
		for (int i = 0; i < 1000; i++)
		{
			glm::vec2 point = Random::Range(glm::vec2{ -100.0f, -100.0f }, glm::vec2{ 100.0f, 100.0f });

			std::vector<entt::entity> nearest = order == CE::Physics::ExploreOrder::NearestFirst ?
				world.GetPhysics().GetSortedNearToFar(point, GetRules()) :
				world.GetPhysics().GetSortedFarToNear(point, GetRules());

			TEST_ASSERT(nearest.size() == sNumCollidersToSpawn);

			const bool hasDuplicates = std::adjacent_find(nearest.begin(), nearest.end()) != nearest.end();
			TEST_ASSERT(!hasDuplicates);

			for (int j = 1; j < nearest.size(); j++)
			{
				float currDist = world.GetPhysics().GetSignedDistance(nearest[j], point);
				float prevDist = world.GetPhysics().GetSignedDistance(nearest[j - 1], point);

				if (order == CE::Physics::ExploreOrder::NearestFirst)
				{
					TEST_ASSERT(prevDist <= currDist);
				}
				else
				{
					TEST_ASSERT(prevDist >= currDist);
				}
			}
		}

		return UnitTest::Result::Success;
	}
}

UNIT_TEST(PhysicsQueries, BVHCheck)
{
	using namespace CE;
	World world{ true };

	PopulateWithRandomColliders(world);

	Registry& reg = world.GetRegistry();
	CollisionRules rules = GetRules();

	world.GetPhysics().UpdateBVHs();

	for (int i = 0; i < 1000; i++)
	{
		TransformedDiskColliderComponent disk{
			Random::Range(glm::vec2{ -100.0f, -100.0f }, glm::vec2{ 100.0f, 100.0f }),
			Random::Range(1.0f, 50.0f),
		};

		std::vector<entt::entity> entities = world.GetPhysics().FindAllWithinShape(disk, rules);

		auto testLambda = [&]<typename T>()
		{
			for (auto [entity, other] : reg.View<T>().each())
			{
				if (AreOverlapping(disk, other))
				{
					auto it = std::find(entities.begin(), entities.end(), entity);
					TEST_ASSERT(it != entities.end());
					entities.erase(it);
				}
			}
			return UnitTest::Result::Success;
		};
		TEST_ASSERT(testLambda.operator()<TransformedDiskColliderComponent>());
		TEST_ASSERT(testLambda.operator()<TransformedAABBColliderComponent>());
		TEST_ASSERT(testLambda.operator()<TransformedPolygonColliderComponent>());

		TEST_ASSERT(entities.empty());
	}

	return UnitTest::Result::Success;
}

UNIT_TEST(PhysicsQueries, NearestCheckSingleFreshBuild)
{
	using namespace CE;
	World world{ true };
	Physics& physics = world.GetPhysics();

	PopulateWithRandomColliders(world);
	physics.UpdateBVHs();

	return ExploreSingle(world, Physics::ExploreOrder::NearestFirst);
}

UNIT_TEST(PhysicsQueries, NearestCheckSingleRefit)
{
	using namespace CE;
	World world{ true };
	Physics& physics = world.GetPhysics();

	PopulateWithRandomColliders(world);
	physics.UpdateBVHs();
	ShuffleColliders(world);
	physics.UpdateBVHs(Physics::UpdateBVHConfig{ .mOnlyRebuildForNewColliders = true });

	return ExploreSingle(world, Physics::ExploreOrder::NearestFirst);
}

UNIT_TEST(PhysicsQueries, FarthestCheckSingleFreshBuild)
{
	using namespace CE;
	World world{ true };
	Physics& physics = world.GetPhysics();

	PopulateWithRandomColliders(world);
	physics.UpdateBVHs();

	return ExploreSingle(world, Physics::ExploreOrder::FarthestFirst);
}

UNIT_TEST(PhysicsQueries, FarthestCheckSingleRefit)
{
	using namespace CE;
	World world{ true };
	Physics& physics = world.GetPhysics();

	PopulateWithRandomColliders(world);
	physics.UpdateBVHs();
	ShuffleColliders(world);
	physics.UpdateBVHs(Physics::UpdateBVHConfig{ .mOnlyRebuildForNewColliders = true });

	return ExploreSingle(world, Physics::ExploreOrder::FarthestFirst);
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiFreshBuild)
{
	using namespace CE;
	World world{ true };
	Physics& physics = world.GetPhysics();

	PopulateWithRandomColliders(world);
	physics.UpdateBVHs();

	return ExploreMulti(world, Physics::ExploreOrder::NearestFirst);
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiRefit)
{
	using namespace CE;
	World world{ true };
	Physics& physics = world.GetPhysics();

	PopulateWithRandomColliders(world);
	physics.UpdateBVHs();
	ShuffleColliders(world);
	physics.UpdateBVHs(Physics::UpdateBVHConfig{ .mOnlyRebuildForNewColliders = true });

	return ExploreMulti(world, Physics::ExploreOrder::NearestFirst);
}

UNIT_TEST(PhysicsQueries, FarthestCheckMultiFreshBuild)
{
	using namespace CE;
	World world{ true };
	Physics& physics = world.GetPhysics();

	PopulateWithRandomColliders(world);
	physics.UpdateBVHs();

	return ExploreMulti(world, Physics::ExploreOrder::FarthestFirst);
}

UNIT_TEST(PhysicsQueries, FarthestCheckMultiRefit)
{
	using namespace CE;
	World world{ true };
	Physics& physics = world.GetPhysics();

	PopulateWithRandomColliders(world);
	physics.UpdateBVHs();
	ShuffleColliders(world);
	physics.UpdateBVHs(Physics::UpdateBVHConfig{ .mOnlyRebuildForNewColliders = true });

	return ExploreMulti(world, Physics::ExploreOrder::FarthestFirst);
}
