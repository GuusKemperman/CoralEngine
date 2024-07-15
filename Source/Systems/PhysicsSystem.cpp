#include "Precomp.h"
#include "Systems/PhysicsSystem.h"

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Meta/MetaType.h"
#include "Components/TransformComponent.h"
#include "Utilities/Geometry2d.h"
#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Rendering/DebugRenderer.h"
#include "Utilities/BVH.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Time.h"
#include "World/Physics.h"

CE::PhysicsSystem::PhysicsSystem() :
	mOnCollisionEntryEvents(GetAllBoundEvents(sOnCollisionEntry)),
	mOnCollisionStayEvents(GetAllBoundEvents(sOnCollisionStay)),
	mOnCollisionExitEvents(GetAllBoundEvents(sOnCollisionExit))
{
}

void CE::PhysicsSystem::Update(World& world, float dt)
{
	if (world.HasBegunPlay()
		&& !world.IsPaused())
	{
		ApplyVelocities(world, dt);
	}

	world.GetPhysics().RebuildBVHs();


	if (world.HasBegunPlay()
		&& !world.IsPaused())
	{
		UpdateCollisions(world);
	}
}

void CE::PhysicsSystem::Render(const World& world)
{
	DebugDrawing(world);

	for (const BVH& bvh : world.GetPhysics().GetBVHs())
	{
		bvh.DebugDraw();
	}
}

void CE::PhysicsSystem::ApplyVelocities(World& world, float dt)
{
	Registry& reg = world.GetRegistry();

	const auto view = reg.View<PhysicsBody2DComponent, TransformComponent>();
	for (auto [entity, body, transform] : view.each())
	{
		if (body.mIsAffectedByForces)
		{
			body.mLinearVelocity += body.mForce * body.mInvMass * dt;
		}

		if (body.mLinearVelocity == glm::vec2{})
		{
			continue;
		}

		transform.TranslateWorldPosition(body.mLinearVelocity * dt);
	}
}

namespace CE::Internal
{
	struct ShouldCheckForCollision
	{
		template<typename ColliderType>
		static bool Callback(entt::entity entity2, entt::entity entity1, const PhysicsBody2DComponent& body1, const Registry& reg)
		{
			if (entity1 == entity2)
			{
				return false;
			}

			const PhysicsBody2DComponent* body2 = reg.TryGet<PhysicsBody2DComponent>(entity2);

			return body2 != nullptr
				&& body1.mRules.GetResponse(body2->mRules) != CollisionResponse::Ignore;
		}

		template<>
		STATIC_SPECIALIZATION bool Callback<TransformedDiskColliderComponent>(entt::entity entity2, entt::entity entity1, const PhysicsBody2DComponent& body1, const Registry& reg)
		{
			if (entity1 >= entity2)
			{
				return false;
			}

			const PhysicsBody2DComponent* body2 = reg.TryGet<PhysicsBody2DComponent>(entity2);

			return body2 != nullptr
				&& body1.mRules.GetResponse(body2->mRules) != CollisionResponse::Ignore;
		}
	};
}

void CE::PhysicsSystem::UpdateCollisions(World& world)
{
	Registry& reg = world.GetRegistry();

	static std::vector<std::pair<entt::entity, entt::entity>> diskDiskCollisions{};
	static std::vector<std::pair<entt::entity, entt::entity>> diskAABBCollisions{};
	static std::vector<std::pair<entt::entity, entt::entity>> diskPolygonCollisions{};

	diskDiskCollisions.clear();
	diskAABBCollisions.clear();
	diskPolygonCollisions.clear();

	CollisionData collision;

	// prev collisions are stored as a member function,
	// but this is made static so we can reuse the same
	// buffer every frame, even if we have multiple worlds.
	static std::vector<CollisionData> currentCollisions{};
	currentCollisions.clear();

	struct OnIntersect
	{
		static void Callback(const TransformedDiskColliderComponent&, entt::entity entity2, entt::entity entity1, const PhysicsBody2DComponent&, const Registry&)
		{
			diskDiskCollisions.emplace_back(entity1, entity2);
		}

		static void Callback(const TransformedAABBColliderComponent&, entt::entity entity2, entt::entity entity1, const PhysicsBody2DComponent&, const Registry&)
		{
			diskAABBCollisions.emplace_back(entity1, entity2);
		}

		static void Callback(const TransformedPolygonColliderComponent&, entt::entity entity2, entt::entity entity1, const PhysicsBody2DComponent&, const Registry&)
		{
			diskPolygonCollisions.emplace_back(entity1, entity2);
		}
	};

	const auto viewDisk = reg.View<PhysicsBody2DComponent, TransformedDiskColliderComponent, TransformComponent>();
	const auto viewPolygon = reg.View<PhysicsBody2DComponent, TransformedPolygonColliderComponent>();
	const auto viewAABB = reg.View<PhysicsBody2DComponent, TransformedAABBColliderComponent>();

	const Physics::BVHS& bvhs = world.GetPhysics().GetBVHs();

	// In the first pass we collect all the collision pairs,
	// but we don't move anything to prevent the BVH from being
	// invalidated.
	for (entt::entity entity1 : viewDisk)
	{
		const auto [body1, disk1] = viewDisk.get<PhysicsBody2DComponent, TransformedDiskColliderComponent>(entity1);

		for (const BVH& bvh : bvhs)
		{
			if (body1.mRules.mResponses[static_cast<int>(bvh.GetLayer())] == CollisionResponse::Ignore)
			{
				continue;
			}

			bvh.Query<OnIntersect, Internal::ShouldCheckForCollision, BVH::DefaultShouldReturnFunction<false>>(disk1, entity1, body1, reg);
		}
	}

	for (auto [entity1, entity2] : diskDiskCollisions)
	{
		auto [body1, transformedDiskCollider1, transform1] = viewDisk.get<PhysicsBody2DComponent, TransformedDiskColliderComponent, TransformComponent>(entity1);
		auto [body2, transformedDiskCollider2, transform2] = viewDisk.get<PhysicsBody2DComponent, TransformedDiskColliderComponent, TransformComponent>(entity2);

		if (!CollisionCheckDiskDisk(transformedDiskCollider1, transformedDiskCollider2, collision))
		{
			continue;
		}

		RegisterCollision(currentCollisions, collision, entity1, entity2);
		const CollisionResponse response = body1.mRules.GetResponse(body2.mRules);

		if (response != CollisionResponse::Blocking)
		{
			continue;
		}

		if (body1.mIsAffectedByForces)
		{
			auto [newEntity1Pos, entity1Impulse] = ResolveDiskCollision(collision, body1, body2, transformedDiskCollider1.mCentre);
			body1.ApplyImpulse(entity1Impulse);
			transform1.SetWorldPosition(newEntity1Pos);
			transformedDiskCollider1.mCentre = newEntity1Pos;
		}

		if (body2.mIsAffectedByForces)
		{
			auto [newEntity2Pos, entity2Impulse] = ResolveDiskCollision(collision, body2, body1, transformedDiskCollider2.mCentre, -1.0f);
			body2.ApplyImpulse(entity2Impulse);
			transform2.SetWorldPosition(newEntity2Pos);
			transformedDiskCollider2.mCentre = newEntity2Pos;
		}
	}

	for (const auto& [entity1, entity2] : diskAABBCollisions)
	{
		auto [transform1, body1, transformedDiskCollider1] = viewDisk.get<TransformComponent, PhysicsBody2DComponent, TransformedDiskColliderComponent>(entity1);
		auto [body2, transformedAABBCollider] = viewAABB.get<PhysicsBody2DComponent, TransformedAABBColliderComponent>(entity2);

		const CollisionResponse response = body1.mRules.GetResponse(body2.mRules);

		if (response == CollisionResponse::Ignore)
		{
			continue;
		}

		if (CollisionCheckDiskAABB(transformedDiskCollider1, transformedAABBCollider, collision))
		{
			RegisterCollision(currentCollisions, collision, entity1, entity2);

			if (response == CollisionResponse::Blocking
				&& body1.mIsAffectedByForces)
			{
				auto [newEntity1Pos, entity1Impulse] = ResolveDiskCollision(collision, body1, body2, transformedDiskCollider1.mCentre);
				body1.ApplyImpulse(entity1Impulse);
				transform1.SetWorldPosition(newEntity1Pos);
				transformedDiskCollider1.mCentre = newEntity1Pos;
			}
		}
	}

	for (auto [entity1, entity2] : diskPolygonCollisions)
	{
		auto [transform1, body1, transformedDiskCollider1] = viewDisk.get<TransformComponent, PhysicsBody2DComponent, TransformedDiskColliderComponent>(entity1);
		auto [body2, transformedPolygonCollider2] = viewPolygon.get<PhysicsBody2DComponent, TransformedPolygonColliderComponent>(entity2);

		const CollisionResponse response = body1.mRules.GetResponse(body2.mRules);

		if (response == CollisionResponse::Ignore)
		{
			continue;
		}

		if (CollisionCheckDiskPolygon(transformedDiskCollider1, transformedPolygonCollider2, collision))
		{
			RegisterCollision(currentCollisions, collision, entity1, entity2);

			if (response == CollisionResponse::Blocking
				&& body1.mIsAffectedByForces)
			{
				auto [newEntity1Pos, entity1Impulse] = ResolveDiskCollision(collision, body1, body2, transformedDiskCollider1.mCentre);
				body1.ApplyImpulse(entity1Impulse);
				transform1.SetWorldPosition(newEntity1Pos);
				transformedDiskCollider1.mCentre = newEntity1Pos;
			}
		}
	}

	static std::vector<std::reference_wrapper<const CollisionData>> enters{};
	static std::vector<std::reference_wrapper<const CollisionData>> exits{};
	enters.clear();
	exits.clear();

	for (const CollisionData& currFrame : currentCollisions)
	{
		const bool wasCollidingPrevFrame = std::any_of(mPreviousCollisions.begin(), mPreviousCollisions.end(),
		                                               [&currFrame](const CollisionData& prevFrame)
		                                               {
			                                               return currFrame.mEntity1 == prevFrame.mEntity1
				                                               && currFrame.mEntity2 == prevFrame.mEntity2;
		                                               });

		if (!wasCollidingPrevFrame)
		{
			enters.emplace_back(currFrame);
		}
	}

	for (const CollisionData& prevFrame : mPreviousCollisions)
	{
		const bool isCollidingNow = std::any_of(currentCollisions.begin(), currentCollisions.end(),
		                                        [&prevFrame](const CollisionData& currFrame)
		                                        {
			                                        return currFrame.mEntity1 == prevFrame.mEntity1
				                                        && currFrame.mEntity2 == prevFrame.mEntity2;
		                                        });

		if (!isCollidingNow)
		{
			exits.emplace_back(prevFrame);
		}
	}

	// Call events
	CallEvents(world, enters, mOnCollisionEntryEvents);
	CallEvents(world, currentCollisions, mOnCollisionStayEvents);
	CallEvents(world, exits, mOnCollisionExitEvents);

	std::swap(mPreviousCollisions, currentCollisions);
}



template <typename CollisionDataContainer>
void CE::PhysicsSystem::CallEvents(World& world, const CollisionDataContainer& collisions,
                                         const std::vector<BoundEvent>& events)
{
	if (collisions.empty())
	{
		return;
	}

	Registry& reg = world.GetRegistry();

	for (const BoundEvent& event : events)
	{
		entt::sparse_set* const storage = reg.Storage(event.mType.get().GetTypeId());

		if (storage == nullptr)
		{
			continue;
		}

		for (const CollisionData& collision : collisions)
		{
			CallEvent(event, world, *storage, collision.mEntity1, collision.mEntity2, collision.mDepth, collision.mNormalFor1, collision.mContactPoint);
			CallEvent(event, world, *storage, collision.mEntity2, collision.mEntity1, collision.mDepth, -collision.mNormalFor1, collision.mContactPoint);
		}
	}
}

void CE::PhysicsSystem::DebugDrawing(const World& world)
{
	const Registry& reg = world.GetRegistry();

	if (DebugRenderer::IsCategoryVisible(DebugCategory::Physics))
	{
		const auto diskView = reg.View<const TransformedDiskColliderComponent, const TransformComponent>();
		constexpr glm::vec4 color = { 1.f, 0.f, 0.f, 1.f };
		for (auto [entity, disk, transform] : diskView.each())
		{
			DrawDebugCircle(world, DebugCategory::Physics, To3DRightForward(disk.mCentre), disk.mRadius + 0.00001f, color);
		}

		const auto polyView = reg.View<const TransformedPolygonColliderComponent, const TransformComponent>();
		for (auto [entity, poly, transform] : polyView.each())
		{
			const size_t pointCount = poly.mPoints.size();
			for (size_t i = 0; i < pointCount; ++i)
			{
				const glm::vec2 from = poly.mPoints[i];
				const glm::vec2 to = poly.mPoints[(i + 1) % pointCount];
				DrawDebugLine(world, DebugCategory::Physics, To3DRightForward(from), To3DRightForward(to), color);
			}
		}

		const auto aabbView = reg.View<const TransformedAABBColliderComponent, const TransformComponent>();
		for (auto [entity, aabb, transform] : aabbView.each())
		{
			DrawDebugRectangle(world, DebugCategory::Physics, To3DRightForward(aabb.GetCentre()), aabb.GetSize() * .5f, color);
		}
	}
}

void CE::PhysicsSystem::CallEvent(const BoundEvent& event, World& world, entt::sparse_set& storage,
        entt::entity owner, entt::entity otherEntity, float depth, glm::vec2 normal, glm::vec2 contactPoint)
{
	// Tombstone check, is needed
	if (!storage.contains(owner))
	{
		return;
	}

	if (event.mIsStatic)
	{
		event.mFunc.get().InvokeUncheckedUnpacked(world, owner, otherEntity, depth, normal, contactPoint);
	}
	else
	{
		MetaAny component{ event.mType, storage.value(owner), false };
		event.mFunc.get().InvokeUncheckedUnpacked(component, world, owner, otherEntity, depth, normal, contactPoint);
	}
}

CE::PhysicsSystem::ResolvedCollision CE::PhysicsSystem::ResolveDiskCollision(const CollisionData& collisionToResolve, 
	const PhysicsBody2DComponent& bodyToMove,
	const PhysicsBody2DComponent& otherBody, 
	const glm::vec2& bodyPosition, 
	float multiplicant)
{
	// displace the objects to resolve overlap
	const float totalInvMass = bodyToMove.mInvMass + otherBody.mInvMass;
	const glm::vec2 dist = (collisionToResolve.mDepth / totalInvMass) * collisionToResolve.mNormalFor1;

	const glm::vec2 resolvedPos = bodyPosition + multiplicant * dist * bodyToMove.mInvMass;

	// compute and apply impulses
	const float dotProduct = dot(bodyToMove.mLinearVelocity - otherBody.mLinearVelocity, collisionToResolve.mNormalFor1);

	glm::vec2 impulse{};

	if (dotProduct <= 0)
	{
		const float restitution = bodyToMove.mRestitution + otherBody.mRestitution;
		const float j = -(1.0f + restitution * 0.5f) * dotProduct / (1.0f / bodyToMove.mInvMass + 1.0f / otherBody.mInvMass);
		impulse = multiplicant * j * collisionToResolve.mNormalFor1;
	}

	return { resolvedPos, impulse };
}

void CE::PhysicsSystem::RegisterCollision(std::vector<CollisionData>& currentCollisions,
                                                CollisionData& collision, entt::entity entity1, entt::entity entity2)
{
	collision.mEntity1 = entity1;
	collision.mEntity2 = entity2;
	currentCollisions.emplace_back(collision);
}

static constexpr glm::vec2 sDefaultNormal =  glm::vec2{ 0.707107f };

bool CE::PhysicsSystem::CollisionCheckDiskDisk(TransformedDiskColliderComponent disk1, TransformedDiskColliderComponent disk2, CollisionData& result)
{
	// check for overlap
	const glm::vec2 diff(disk1.mCentre - disk2.mCentre);
	const float l2 = length2(diff);
	const float r = disk1.mRadius + disk2.mRadius;

	if (l2 > r * r)
	{
		return false;
	}

	const float l = sqrt(l2);

	// compute collision details
	result.mDepth = r - l;

	if (l != 0.0f)
	{
		result.mNormalFor1 = diff / l;
	}
	else
	{
		result.mNormalFor1 = sDefaultNormal;
	}

	result.mContactPoint = disk2.mCentre + result.mNormalFor1 * disk2.mRadius;

	return true;
}

bool CE::PhysicsSystem::CollisionCheckDiskPolygon(TransformedDiskColliderComponent disk, const TransformedPolygonColliderComponent& polygon, CollisionData& result)
{
	if (!AreOverlapping(disk, polygon.mBoundingBox))
	{
		return false;
	}

	glm::vec2 nearest = GetNearestPointOnPolygonBoundary(disk.mCentre, polygon.mPoints);
	const glm::vec2 diff(disk.mCentre - nearest);
	const float l2 = length2(diff);

	if (AreOverlapping(disk.mCentre, polygon))
	{
		const float l = sqrt(l2);

		if (l != 0.0f)
		{
			result.mNormalFor1 = -diff / l;
		}
		else
		{
			result.mNormalFor1 = sDefaultNormal;
		}

		result.mDepth = l + disk.mRadius;
		return true;
	}

	if (l2 > disk.mRadius * disk.mRadius) return false;

	// compute collision details
	const float l = sqrt(l2);

	if (l != 0.0f)
	{
		result.mNormalFor1 = diff / l;
	}
	else
	{
		result.mNormalFor1 = sDefaultNormal;
	}

	result.mDepth = disk.mRadius - l;
	result.mContactPoint = nearest;
	return true;
}

bool CE::PhysicsSystem::CollisionCheckDiskAABB(TransformedDiskColliderComponent disk, TransformedAABBColliderComponent aabb, CollisionData& result)
{
	if (!AreOverlapping(disk, aabb))
	{
		return false;
	}

	return CollisionCheckDiskPolygon(disk, aabb.GetAsPolygon(), result);
}

CE::MetaType CE::PhysicsSystem::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PhysicsSystem>{}, "PhysicsSystem", MetaType::Base<System>{} };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc([](entt::entity entity1, entt::entity entity2, float depth, glm::vec2 normalFor1, glm::vec2 contactPoint, const PhysicsBody2DComponent& bodyToMove, const PhysicsBody2DComponent& otherBody, const glm::vec2& bodyPosition) -> glm::vec2
		{
			return ResolveDiskCollision({ entity1 , entity2, depth, normalFor1, contactPoint }, bodyToMove, otherBody, bodyPosition).mResolvedPosition;
		},
		"ResolveCollision", MetaFunc::ExplicitParams<entt::entity , entt::entity , float, glm::vec2, glm::vec2, const PhysicsBody2DComponent&, const PhysicsBody2DComponent&, glm::vec2>{}, 
		"Entity1", "Entity2", "Depth", "NormalFor1", "ContactPoint", "PhysicsBodyToMove", "OtherPhysicsBody", "BodyPosition").GetProperties().Add(Props::sIsScriptableTag);

	return metaType;
}
