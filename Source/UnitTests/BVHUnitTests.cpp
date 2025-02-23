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

UNIT_TEST(BVH, BVHCheck)
{
	using namespace CE;
	World world{ true };

	Registry& reg = world.GetRegistry();

	CollisionRules rules{};
	rules.mLayer = CollisionLayer::Query;
	rules.SetResponse(CollisionLayer::Query, CollisionResponse::Overlap);

	auto addLambda = [&]<typename T>(const T& collider)
		{
			for (int i = 0; i < 100; i++)
			{
				entt::entity entity = reg.Create();
				reg.AddComponent<TransformComponent>(entity).SetWorldPosition(CE::Random::Range(glm::vec2{ -100.0f, -100.0f }, glm::vec2{ -1000.0f, -1000.0f }));
				reg.AddComponent<PhysicsBody2DComponent>(entity).mRules = rules;
				reg.AddComponent<T>(entity, collider);
			}
		};
	addLambda(DiskColliderComponent{ 5.0f });
	addLambda(AABBColliderComponent{ glm::vec2{ 5.0f, 5.0f } });
	addLambda(PolygonColliderComponent{ { glm::vec2{ -5.0f, 0.0f}, glm::vec2{ 0.0f, 5.0f}, glm::vec2{ 5.0f, 0.0f} } });

	world.GetPhysics().UpdateBVHs();

	for (int i = 0; i < 1000; i++)
	{
		TransformedDiskColliderComponent disk{
			Random::Range(glm::vec2{ -100.0f, -1000.0f }, glm::vec2{ -100.0f, -1000.0f }),
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