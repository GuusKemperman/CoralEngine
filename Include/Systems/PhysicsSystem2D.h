#pragma once
#include "Systems/System.h"

struct Physics2DUnitTestAccess;

namespace Engine
{
	class Registry;
	class MetaFunc;
	class PhysicsBody2DComponent;

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

		/// <summary>
		/// Stores the details of a single collision.
		/// </summary>
		struct CollisionData
		{
			entt::entity mEntity1{};

			/// The ID of the other entity involved in the collision.
			entt::entity mEntity2{};

			/// The penetration depth of the two physics bodies
			/// (before they were displaced to resolve overlap).
			float mDepth{};

			/// The normal vector on the point of contact, pointing away from entity2's physics body.
			glm::vec2 mNormalFor1{};

			/// The approximate point of contact of the collision, in world coordinates.
			glm::vec2 mContactPoint{};
		};

		void UpdateBodiesAndTransforms(World& world, float dt);
		void UpdateCollisions(World& world);
		void DebugDrawing(World& world);

		struct CollisionEvent
		{
			std::reference_wrapper<const MetaType> mComponentType;
			std::reference_wrapper<const MetaFunc> mEvent;
			bool mIsStatic{};
		};


		template<typename CollisionDataContainer>
		static void CallEvents(World& world, const CollisionDataContainer& collisions, const std::vector<CollisionEvent>& events);

		static void CallEvent(const CollisionEvent& event, World& world, entt::sparse_set& storage, entt::entity owner, entt::entity otherEntity, float depth, glm::vec2 normal, glm::vec2 contactPoint);

		static void PrintCollisionData(entt::entity entity, const PhysicsBody2DComponent& body);
		static void ResolveCollision(const CollisionData& collision, PhysicsBody2DComponent& body1,
			PhysicsBody2DComponent& body2);

		//static void RegisterCollision(CollisionData& collision, const entt::entity& entity1,
		//	PhysicsBody2DComponent& body1, const entt::entity& entity2,
		//	PhysicsBody2DComponent& body2);

		static bool CollisionCheckDiskDisk(const glm::vec2& center1, float radius1, const glm::vec2& center2,
			float radius2, CollisionData& result);

		static bool CollisionCheckDiskPolygon(const glm::vec2& diskCenter, float diskRadius,
			const glm::vec2& polygonPos, const std::vector<glm::vec2>& polygonPoints,
			CollisionData& result);

		static bool IsPointInsidePolygon(const glm::vec2& point, const std::vector<glm::vec2>& polygon);

		static glm::vec2 GetNearestPointOnPolygonBoundary(const glm::vec2& point, const std::vector<glm::vec2>& polygon);

		static glm::vec2 GetNearestPointOnLineSegment(const glm::vec2& p, const glm::vec2& segmentA,
			const glm::vec2& segmentB);

		std::vector<CollisionData> mPreviousCollisions{};


		std::vector<CollisionEvent> mOnCollisionEntryEvents{};
		std::vector<CollisionEvent> mOnCollisionStayEvents{};
		std::vector<CollisionEvent> mOnCollisionExitEvents{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsSystem2D);
	};


}
