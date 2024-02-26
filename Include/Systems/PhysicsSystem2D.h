#pragma once
#include "Systems/System.h"

namespace Engine
{
	class PhysicsBody2DComponent;
	struct CollisionData;

	class PhysicsSystem2D final :
		public System
	{
	public:
		void Update(World& world, float dt) override;

	private:
		void UpdateBodiesAndTransforms(World& world, float dt);
		void CheckAndRegisterCollisions(World& world);
		void DebugDrawing(World& world);

		void PrintCollisionData(entt::entity entity, PhysicsBody2DComponent& body);

		void RegisterCollision(CollisionData& collision, const entt::entity& entity1, PhysicsBody2DComponent& body1, const entt::entity& entity2, PhysicsBody2DComponent& body2);
		bool CollisionCheckDiskDisk(const glm::vec2& center1, float radius1, const glm::vec2& center2, float radius2, CollisionData& result);
		bool CollisionCheckDiskPolygon(const glm::vec2& diskCenter, float diskRadius, const glm::vec2& polygonPos, const std::vector<glm::vec2>& polygonPoints, CollisionData& result);
		bool IsPointInsidePolygon(const glm::vec2& point, const std::vector<glm::vec2>& polygon);
		glm::vec2 GetNearestPointOnPolygonBoundary(const glm::vec2& point, const std::vector<glm::vec2>& polygon);
		glm::vec2 GetNearestPointOnLineSegment(const glm::vec2& p, const glm::vec2& segmentA, const glm::vec2& segmentB);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsSystem2D);
	};
}
