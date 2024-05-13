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
#include "World/Physics.h"

CE::PhysicsSystem::PhysicsSystem() :
	mOnCollisionEntryEvents(GetAllBoundEvents(sCollisionEntryEvent)),
	mOnCollisionStayEvents(GetAllBoundEvents(sCollisionStayEvent)),
	mOnCollisionExitEvents(GetAllBoundEvents(sCollisionExitEvent))
{
}

void CE::PhysicsSystem::Update(World& world, float dt)
{
	if (world.HasBegunPlay()
		&& !world.IsPaused())
	{
		ApplyVelocities(world, dt);
	}

	UpdateTransformedColliders<DiskColliderComponent, TransformedDiskColliderComponent>(world);
	UpdateTransformedColliders<AABBColliderComponent, TransformedAABBColliderComponent>(world);
	UpdateTransformedColliders<PolygonColliderComponent, TransformedPolygonColliderComponent>(world);

	world.GetPhysics().GetBVH().Build();

	if (world.HasBegunPlay()
		&& !world.IsPaused())
	{
		UpdateCollisions(world);
	}

}

void CE::PhysicsSystem::Render(const World& world)
{
	DebugDrawing(world);
	world.GetPhysics().GetBVH().DebugDraw();
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

void CE::PhysicsSystem::UpdateCollisions(World& world)
{
	Registry& reg = world.GetRegistry();

	const auto viewDisk = reg.View<TransformComponent, PhysicsBody2DComponent, TransformedDiskColliderComponent>();
	const auto viewPolygon = reg.View<const PhysicsBody2DComponent, const TransformedPolygonColliderComponent>();
	const auto viewAABB = reg.View<const PhysicsBody2DComponent, const TransformedAABBColliderComponent>();

	CollisionData collision;

	// prev collisions are stored as a member function,
	// but this is made static so we can reuse the same
	// buffer every frame, even if we have multiple worlds.
	static std::vector<CollisionData> currentCollisions{};
	currentCollisions.clear();

	// collisions between a dynamic and static/kinematic body are only resolved if the dynamic body is a disk
	for (auto it1 = viewDisk.begin(); it1 != viewDisk.end(); ++it1)
	{
		const entt::entity entity1 = *it1;
		auto [transform1, body1, transformedDiskCollider1] = viewDisk.get<TransformComponent, PhysicsBody2DComponent, TransformedDiskColliderComponent>(entity1);

		// Workaround, because ***REMOVED*** doesn't like it otherwise
		TransformedDiskColliderComponent* pointerToTransformedDiskCollider1 = &transformedDiskCollider1;

		// Can be modified by ResolveCollision. Is only actually applied
		// to the transform at the end of this loop, for performance
		// reasons.
		glm::vec2 entity1Pos = transform1.GetWorldPosition2D();
		const glm::vec2 entity1PosAtStart = entity1Pos;
		glm::vec2 entity1TotalImpulse{};

		auto resolveCollisionFor1 = [&pointerToTransformedDiskCollider1, &entity1Pos, &entity1TotalImpulse](ResolvedCollision resolvedCollision)
			{
				entity1TotalImpulse += resolvedCollision.mImpulse;
				entity1Pos = resolvedCollision.mResolvedPosition;
				pointerToTransformedDiskCollider1->mCentre = entity1Pos;
			};

		// disk-disk collisions
		for (auto it2 = [&it1]{ auto tmp = it1; ++tmp; return tmp; }(); it2 != viewDisk.end(); ++it2)
		{
			const entt::entity entity2 = *it2;
			ASSERT(entity1 != entity2);
			auto [transform2, body2, transformedDiskCollider2] = viewDisk.get<TransformComponent, PhysicsBody2DComponent, TransformedDiskColliderComponent>(entity2);

			const CollisionResponse response = body1.mRules.GetResponse(body2.mRules);

			if (response == CollisionResponse::Ignore)
			{
				continue;
			}

			const glm::vec2 entity2Pos = transformedDiskCollider2.mCentre;

			if (CollisionCheckDiskDisk(transformedDiskCollider1, transformedDiskCollider2, collision))
			{
				RegisterCollision(currentCollisions, collision, entity1, entity2);

				if (response == CollisionResponse::Blocking)
				{
					if (body1.mIsAffectedByForces)
					{
						resolveCollisionFor1(ResolveDiskCollision(collision, body1, body2, entity1Pos));
					}

					if (body2.mIsAffectedByForces)
					{
						auto [newEntity2Pos, entity2Impulse] = ResolveDiskCollision(collision, body2, body1, entity2Pos, -1.0f);
						body2.ApplyImpulse(entity2Impulse);
						transform2.SetWorldPosition(newEntity2Pos);
						transformedDiskCollider2.mCentre = newEntity2Pos;
					}
				}
			}
		}

		// disk-polygon collisions
		for (const auto [entity2, body2, transformedPolygonCollider] : viewPolygon.each())
		{
			const CollisionResponse response = body1.mRules.GetResponse(body2.mRules);

			if (response == CollisionResponse::Ignore)
			{
				continue;
			}

			if (CollisionCheckDiskPolygon(transformedDiskCollider1, transformedPolygonCollider, collision))
			{
				RegisterCollision(currentCollisions, collision, entity1, entity2);

				if (response == CollisionResponse::Blocking
					&& body1.mIsAffectedByForces)
				{
					resolveCollisionFor1(ResolveDiskCollision(collision, body1, body2, entity1Pos));
				}
			}
		}

		// disk-aabb collisions
		for (const auto [entity2, body2, transformedAABBCollider] : viewAABB.each())
		{
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
					resolveCollisionFor1(ResolveDiskCollision(collision, body1, body2, entity1Pos));
				}
			}
		}

		if (entity1Pos != entity1PosAtStart)
		{
			transform1.SetWorldPosition(entity1Pos);
		}

		body1.ApplyImpulse(entity1TotalImpulse);
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

template <typename Collider, typename TransformedCollider>
void CE::PhysicsSystem::UpdateTransformedColliders(World& world)
{
	Registry& reg = world.GetRegistry();
	const auto collidersWithoutTransformed = reg.View<Collider>(entt::exclude_t<TransformedCollider>{});
	reg.AddComponents<TransformedCollider>(collidersWithoutTransformed.begin(), collidersWithoutTransformed.end());

	const auto transformedWithoutColliders = reg.View<TransformedCollider>(entt::exclude_t<Collider>{});
	reg.RemoveComponents<TransformedCollider>(transformedWithoutColliders.begin(), transformedWithoutColliders.end());

	const auto colliderView = reg.View<TransformComponent, Collider, TransformedCollider>();

	for (auto [entity, transform, collider, transformedCollider] : colliderView.each())
	{
		transformedCollider = collider.CreateTransformedCollider(transform);
	}
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
			DrawDebugCircle(world, DebugCategory::Physics, To3DRightForward(disk.mCentre, transform.GetWorldPosition()[Axis::Up]), disk.mRadius + 0.00001f, color);
		}

		const auto polyView = reg.View<const TransformedPolygonColliderComponent, const TransformComponent>();
		for (auto [entity, poly, transform] : polyView.each())
		{
			float height = transform.GetWorldPosition()[Axis::Up];

			const size_t pointCount = poly.mPoints.size();
			for (size_t i = 0; i < pointCount; ++i)
			{
				const glm::vec2 from = poly.mPoints[i];
				const glm::vec2 to = poly.mPoints[(i + 1) % pointCount];
				DrawDebugLine(world, DebugCategory::Physics, To3DRightForward(from, height), To3DRightForward(to, height), color);
			}
		}

		const auto aabbView = reg.View<const TransformedAABBColliderComponent, const TransformComponent>();
		for (auto [entity, aabb, transform] : aabbView.each())
		{
			DrawDebugRectangle(world, DebugCategory::Physics, To3DRightForward(aabb.GetCentre(), transform.GetWorldPosition()[Axis::Up]), aabb.GetSize() * .5f, color);
		}
	}
}

void CE::PhysicsSystem::CallEvent(const BoundEvent& event, World& world, entt::sparse_set& storage,
        entt::entity owner, entt::entity otherEntity, float depth, glm::vec2 normal, glm::vec2 contactPoint)
{
	static_assert(std::is_same_v<decltype(sCollisionEntryEvent), const Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)>>);
	static_assert(std::is_same_v<decltype(sCollisionStayEvent), const Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)>>);
	static_assert(std::is_same_v<decltype(sCollisionExitEvent), const Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)>>);

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
	return MetaType{ MetaType::T<PhysicsSystem>{}, "PhysicsSystem", MetaType::Base<System>{} };
}
