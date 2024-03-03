#pragma once
#include "Systems/System.h"

struct Physics2DUnitTestAccess;

namespace Engine
{
	class PhysicsBody2DComponent;
	struct CollisionData;

	class PhysicsSystem2D final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::Physics);
			return traits;
		}

	private:
		friend Physics2DUnitTestAccess;

		void UpdateBodiesAndTransforms(World& world, float dt);
		void UpdateCollisions(World& world);
		void DebugDrawing(World& world);

		static void PrintCollisionData(entt::entity entity, const PhysicsBody2DComponent& body);
		static void ResolveCollision(const CollisionData& collision, PhysicsBody2DComponent& body1,
		                             PhysicsBody2DComponent& body2);

		static void RegisterCollision(CollisionData& collision, const entt::entity& entity1,
		                              PhysicsBody2DComponent& body1, const entt::entity& entity2,
		                              PhysicsBody2DComponent& body2);
		static bool CollisionCheckDiskDisk(const glm::vec2& center1, float radius1, const glm::vec2& center2,
		                                   float radius2, CollisionData& result);
		static bool CollisionCheckDiskPolygon(const glm::vec2& diskCenter, float diskRadius,
		                                      const glm::vec2& polygonPos, const std::vector<glm::vec2>& polygonPoints,
		                                      CollisionData& result);
		static bool IsPointInsidePolygon(const glm::vec2& point, const std::vector<glm::vec2>& polygon);
		static glm::vec2
		GetNearestPointOnPolygonBoundary(const glm::vec2& point, const std::vector<glm::vec2>& polygon);
		static glm::vec2 GetNearestPointOnLineSegment(const glm::vec2& p, const glm::vec2& segmentA,
		                                              const glm::vec2& segmentB);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsSystem2D);
	};
}
