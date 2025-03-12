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
	constexpr int sNumCollidersToSpawn = 600;
	constexpr int sNumIterationsPerTest = 10;

	CollisionRules GetRules()
	{
		CollisionRules rules{};
		rules.mLayer = CollisionLayer::Query;
		rules.SetResponse(CollisionLayer::Query, CollisionResponse::Overlap);
			return rules;
	}

	void PopulateWithRandomColliders(CE::World& world, bool disks = true, bool aabbs = true, bool polys = true)
	{
		Registry& reg = world.GetRegistry();

		CollisionRules rules = GetRules();

		auto addLambda = [&]<typename T>(const T& collider)
		{
			for (int i = 0; i < sNumCollidersToSpawn / 3; i++)
			{
				entt::entity entity = reg.Create();
				reg.AddComponent<TransformComponent>(entity).SetWorldPosition(CE::Random::Range(glm::vec2{ -50.0f }, glm::vec2{ 50.0f }));
				reg.AddComponent<PhysicsBody2DComponent>(entity).mRules = rules;
				reg.AddComponent<T>(entity, collider);
			}
		};

		if (disks)
		{
			addLambda(DiskColliderComponent{ 5.0f });
		}

		if (aabbs)
		{
			addLambda(AABBColliderComponent{ glm::vec2{ 5.0f, 5.0f } });
		}

		if (polys)
		{
			addLambda(PolygonColliderComponent{ { glm::vec2{ -5.0f, 0.0f}, glm::vec2{ 0.0f, 5.0f}, glm::vec2{ 5.0f, 2.5f} } });
		}

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
			};
			testLambda.operator()<TransformedDiskColliderComponent>();
			testLambda.operator()<TransformedAABBColliderComponent>();
			testLambda.operator()<TransformedPolygonColliderComponent>();
		}

		return UnitTest::Result::Success;
	}

	void ExploreMulti(CE::World& world, CE::Physics::ExploreOrder order)
	{
		for (int i = 0; i < 1000; i++)
		{
			glm::vec2 point = Random::Range(glm::vec2{ -100.0f, -100.0f }, glm::vec2{ 100.0f, 100.0f });

			std::vector<entt::entity> nearest = order == CE::Physics::ExploreOrder::NearestFirst ?
				world.GetPhysics().GetSortedNearToFar(point, GetRules()) :
				world.GetPhysics().GetSortedFarToNear(point, GetRules());

			TEST_ASSERT(nearest.size() == world.GetRegistry().View<PhysicsBody2DComponent>().size());

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
	}

	struct ExploreTestParams
	{
		bool mNearestFirst = true;
		bool mMulti{};
		bool mRefit{};
		bool mDisks = true;
		bool mAABBs = true;
		bool mPolys = false;
	};

	void ExploreTest(ExploreTestParams params)
	{
		for (int iteration = 0; iteration < sNumIterationsPerTest; iteration++)
		{
			using namespace CE;
			World world{ true };
			Physics& physics = world.GetPhysics();

			PopulateWithRandomColliders(world, params.mDisks, params.mAABBs, params.mPolys);
			physics.UpdateBVHs();

			if (params.mRefit)
			{
				ShuffleColliders(world);
				physics.UpdateBVHs(Physics::UpdateBVHConfig{ .mOnlyRebuildForNewColliders = true });
			}

			CE::Physics::ExploreOrder order = params.mNearestFirst ? Physics::ExploreOrder::NearestFirst : Physics::ExploreOrder::FarthestFirst;

			if (params.mMulti)
			{
				ExploreMulti(world, order);
			}
			else
			{
				ExploreSingle(world, order);

			}
		}
	}
}

UNIT_TEST(PhysicsQueries, BVHCheck)
{
	for (int iteration = 0; iteration < sNumIterationsPerTest; iteration++)
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
			};
			testLambda.operator() < TransformedDiskColliderComponent > ();
			testLambda.operator() < TransformedAABBColliderComponent > ();
			testLambda.operator() < TransformedPolygonColliderComponent > ();

			TEST_ASSERT(entities.empty());
		}
	}
}

UNIT_TEST(PhysicsQueries, NearestCheckSingleFreshBuild)
{
	ExploreTest({ .mMulti = false,});
}

UNIT_TEST(PhysicsQueries, NearestCheckSingleRefit)
{
	ExploreTest({ .mMulti = false, .mRefit = true });
}

UNIT_TEST(PhysicsQueries, NearestCheckSingleFreshBuildDisks)
{
	ExploreTest({ .mMulti = false, .mAABBs = false, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, NearestCheckSingleRefitDisks)
{
	ExploreTest({ .mMulti = false, .mRefit = true, .mAABBs = false, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiFreshBuild)
{
	ExploreTest({ .mMulti = true, });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiRefit)
{
	ExploreTest({ .mMulti = true, .mRefit = true });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiFreshBuildAABBs)
{
	ExploreTest({ .mMulti = true, .mDisks = false, .mAABBs = true, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiRefitAABBs)
{
	ExploreTest({ .mMulti = true, .mRefit = true, .mDisks = false, .mAABBs = true, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiFreshBuildPolys)
{
	ExploreTest({ .mMulti = true, .mDisks = false, .mAABBs = false, .mPolys = true });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiRefitPolys)
{
	ExploreTest({ .mMulti = true, .mRefit = true, .mDisks = false, .mAABBs = false, .mPolys = true });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiFreshBuildPolysDisks)
{
	ExploreTest({ .mMulti = true, .mDisks = true, .mAABBs = false, .mPolys = true });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiRefitPolysDisks)
{
	ExploreTest({ .mMulti = true, .mRefit = true, .mDisks = true, .mAABBs = false, .mPolys = true });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiFreshBuildPolysAABBs)
{
	ExploreTest({ .mMulti = true, .mDisks = false, .mAABBs = true, .mPolys = true });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiRefitPolysAABBs)
{
	ExploreTest({ .mMulti = true, .mRefit = true, .mDisks = false, .mAABBs = true, .mPolys = true });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiFreshBuildDisksAABBs)
{
	ExploreTest({ .mMulti = true, .mDisks = true, .mAABBs = true, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiRefitDisksAABBs)
{
	ExploreTest({ .mMulti = true, .mRefit = true, .mDisks = true, .mAABBs = true, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiFreshBuildDisks)
{
	ExploreTest({ .mMulti = true, .mAABBs = false, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, NearestCheckMultiRefitDisks)
{
	ExploreTest({ .mMulti = true, .mRefit = true, .mAABBs = false, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, FarthestCheckSingleFreshBuild)
{
	ExploreTest({ .mNearestFirst = false, .mMulti = false, });
}

UNIT_TEST(PhysicsQueries, FarthestCheckSingleRefit)
{
	ExploreTest({ .mNearestFirst = false, .mMulti = false, .mRefit = true });
}

UNIT_TEST(PhysicsQueries, FarthestCheckSingleFreshBuildDisks)
{
	ExploreTest({ .mNearestFirst = false, .mMulti = false, .mAABBs = false, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, FarthestCheckSingleRefitDisks)
{
	ExploreTest({ .mNearestFirst = false, .mMulti = false, .mRefit = true, .mAABBs = false, .mPolys = false });
}

UNIT_TEST(PhysicsQueries, FarthestCheckMultiFreshBuild)
{
	ExploreTest({ .mNearestFirst = false, .mMulti = true, });
}

UNIT_TEST(PhysicsQueries, FarthestCheckMultiRefit)
{
	ExploreTest({ .mNearestFirst = false, .mMulti = true, .mRefit = true });
}

UNIT_TEST(PhysicsQueries, FarthestCheckMultiFreshBuildDisks)
{
	ExploreTest({ .mNearestFirst = false, .mMulti = true, .mAABBs = false, .mPolys = false });
}
