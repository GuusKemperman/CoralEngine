#pragma once
#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Systems/System.h"
#include "Utilities/Events.h"

struct Physics2DUnitTestAccess;

namespace CE
{
	class MetaFunc;
	class PhysicsBody2DComponent;

	class PhysicsSystem final :
		public System
	{
	public:
		PhysicsSystem();

		void Update(World& world, float dt) override;

		void Render(const World& world) override;

		SystemStaticTraits GetStaticTraits() const override
		{
			SystemStaticTraits traits{};
			traits.mPriority = static_cast<int>(TickPriorities::Physics);
			traits.mShouldTickBeforeBeginPlay = true;
			traits.mShouldTickWhilstPaused = true;
			return traits;
		}

	private:
		void ApplyVelocities(World& world, float dt);

		template<typename Collider, typename TransformedCollider>
		void UpdateTransformedColliders(World& world);

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

		void UpdateCollisions(World& world);
		void DebugDrawing(const World& world);

		template<typename CollisionDataContainer>
		static void CallEvents(World& world, const CollisionDataContainer& collisions, const std::vector<BoundEvent>& events);

		static void CallEvent(const BoundEvent& event, World& world, entt::sparse_set& storage, entt::entity owner, entt::entity otherEntity, float depth, glm::vec2 normal, glm::vec2 contactPoint);

		struct ResolvedCollision
		{
			glm::vec2 mResolvedPosition{};
			glm::vec2 mImpulse{};
		};
		static ResolvedCollision ResolveDiskCollision(const CollisionData& collisionToResolve,
			const PhysicsBody2DComponent& bodyToMove,
			const PhysicsBody2DComponent& otherBody,
			const glm::vec2& bodyPosition,
			float multiplicant = 1.0f);

		void RegisterCollision(std::vector<CollisionData>& currentCollisions,
			CollisionData& collision, entt::entity entity1, entt::entity entity2);

		static bool CollisionCheckDiskDisk(TransformedDiskColliderComponent disk1, TransformedDiskColliderComponent disk2, CollisionData& result);

		static bool CollisionCheckDiskPolygon(TransformedDiskColliderComponent disk, const TransformedPolygonColliderComponent& polygon, CollisionData& result);

		static bool CollisionCheckDiskAABB(TransformedDiskColliderComponent disk, TransformedAABBColliderComponent aabb, CollisionData& result);

		std::vector<CollisionData> mPreviousCollisions{};

		std::vector<BoundEvent> mOnCollisionEntryEvents{};
		std::vector<BoundEvent> mOnCollisionStayEvents{};
		std::vector<BoundEvent> mOnCollisionExitEvents{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PhysicsSystem);
	};
}
