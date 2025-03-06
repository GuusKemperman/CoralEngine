#include "Precomp.h"
#include "World/Physics.h"

#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/BVH.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Overload.h"
#include "World/EventManager.h"

CE::Physics::Physics(World& world) :
	mWorld(world),
	mBVHs{
		BVH{ *this, static_cast<CollisionLayer>(0) },
		BVH{ *this, static_cast<CollisionLayer>(1) },
		BVH{ *this, static_cast<CollisionLayer>(2) },
		BVH{ *this, static_cast<CollisionLayer>(3) },
		BVH{ *this, static_cast<CollisionLayer>(4) },
		BVH{ *this, static_cast<CollisionLayer>(5) },
		BVH{ *this, static_cast<CollisionLayer>(6) },
		BVH{ *this, static_cast<CollisionLayer>(7) },
		BVH{ *this, static_cast<CollisionLayer>(8) },
		BVH{ *this, static_cast<CollisionLayer>(9) },
		BVH{ *this, static_cast<CollisionLayer>(10) },
		BVH{ *this, static_cast<CollisionLayer>(11) },
		BVH{ *this, static_cast<CollisionLayer>(12) },
		BVH{ *this, static_cast<CollisionLayer>(13) },
		BVH{ *this, static_cast<CollisionLayer>(14) },
		BVH{ *this, static_cast<CollisionLayer>(15) },
	}
{

}

CE::Physics::~Physics() = default;

template <typename T>
std::vector<entt::entity> CE::Physics::FindAllWithinShapeImpl(const T& shape, const CollisionRules& filter) const
{
	std::vector<entt::entity> ret{};

	Query(
		shape, 
		filter, 
		[&](const auto&, entt::entity entity)
		{
			ret.emplace_back(entity);
		},
		BVH::DefaultShouldCheckFunction<true>{},
		BVH::DefaultShouldReturnFunction<false>{});

	return ret;
}

void CE::Physics::Update(float deltaTime)
{
	if (GetWorld().HasBegunPlay()
		&& !GetWorld().IsPaused())
	{
		ApplyVelocities(deltaTime);
	}

	SyncWorldToPhysics();
	UpdateBVHs();

	if (GetWorld().HasBegunPlay()
		&& !GetWorld().IsPaused())
	{
		ResolveCollisions();
	}
}

void CE::Physics::ApplyVelocities(float deltaTime)
{
	Registry& reg = GetWorld().GetRegistry();

	const auto view = reg.View<PhysicsBody2DComponent, TransformComponent>();
	for (auto [entity, body, transform] : view.each())
	{
		if (body.mIsAffectedByForces)
		{
			body.mLinearVelocity += body.mForce * body.mInvMass * deltaTime;
		}

		if (body.mLinearVelocity == glm::vec2{})
		{
			continue;
		}

		transform.SetWorldPosition(To2D(transform.GetWorldPosition()) + body.mLinearVelocity * deltaTime);
	}
}

void CE::Physics::SyncWorldToPhysics()
{
	auto updateColliderType = [this]<typename Collider, typename TransformedCollider>()
	{
		Registry& reg = GetWorld().GetRegistry();
		const auto collidersWithoutTransformed = reg.View<const PhysicsBody2DComponent,
			const Collider>(entt::exclude_t<TransformedCollider>{});

		for (const entt::entity entity : collidersWithoutTransformed)
		{
			const PhysicsBody2DComponent& body = collidersWithoutTransformed.template get<PhysicsBody2DComponent>(entity);

			GetBVHs()[static_cast<int>(body.mRules.mLayer)].MakeDirty();

			if (!IsCollisionLayerStatic(body.mRules.mLayer))
			{
				reg.AddComponent<TransformedCollider>(entity);
				continue;
			}

			const TransformComponent* transform = reg.TryGet<const TransformComponent>(entity);

			if (transform == nullptr)
			{
				continue;
			}

			const Collider& collider = collidersWithoutTransformed.template get<Collider>(entity);
			reg.AddComponent<TransformedCollider>(entity, collider.CreateTransformedCollider(*transform));
		}

		const auto transformedWithoutColliders = reg.View<TransformedCollider>(entt::exclude_t<Collider>{});
		reg.RemoveComponents<TransformedCollider>(transformedWithoutColliders.begin(), transformedWithoutColliders.end());

		const auto colliderView = reg.View<PhysicsBody2DComponent, TransformComponent, Collider, TransformedCollider>();

		for (auto [entity, body, transform, collider, transformedCollider] : colliderView.each())
		{
			if (IsCollisionLayerStatic(body.mRules.mLayer)
				&& GetWorld().HasBegunPlay()) // We still update static colliders in the editor
			{
				continue;
			}

			transformedCollider = collider.CreateTransformedCollider(transform);
		}
	};

	updateColliderType.operator()<DiskColliderComponent, TransformedDiskColliderComponent>();
	updateColliderType.operator()<AABBColliderComponent, TransformedAABBColliderComponent>();
	updateColliderType.operator()<PolygonColliderComponent, TransformedPolygonColliderComponent>();
}

void CE::Physics::UpdateBVHs(UpdateBVHConfig config)
{
	for (int i = 0; i < static_cast<int>(CollisionLayer::NUM_OF_LAYERS); i++)
	{
		BVH& bvh = mWorld.get().GetPhysics().GetBVHs()[i];

		if (config.mForceRebuild
			|| bvh.IsDirty()
			|| (!config.mOnlyRebuildForNewColliders && bvh.GetAmountRefitted() > config.mMaxAmountRefitBeforeRebuilding))
		{
			bvh.Build();
		}
		else
		{
			bvh.Refit();
		}
	}
}

void CE::Physics::ResolveCollisions()
{
	Registry& reg = GetWorld().GetRegistry();

	thread_local std::vector<std::pair<entt::entity, entt::entity>> diskDiskCollisions{};
	thread_local std::vector<std::pair<entt::entity, entt::entity>> diskAABBCollisions{};
	thread_local std::vector<std::pair<entt::entity, entt::entity>> diskPolygonCollisions{};

	diskDiskCollisions.clear();
	diskAABBCollisions.clear();
	diskPolygonCollisions.clear();

	// prev collisions are stored as a member function,
	// but this is made static so we can reuse the same
	// buffer every frame, even if we have multiple worlds.
	thread_local std::vector<CollisionData> currentCollisions{};
	currentCollisions.clear();

	const auto& bodyStorage = reg.Storage<PhysicsBody2DComponent>();

	const auto viewDisk = reg.View<PhysicsBody2DComponent, TransformedDiskColliderComponent>();
	const auto viewPolygon = reg.View<PhysicsBody2DComponent, TransformedPolygonColliderComponent>();
	const auto viewAABB = reg.View<PhysicsBody2DComponent, TransformedAABBColliderComponent>();

	const BVHS& bvhs = GetBVHs();

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

			bvh.Query(
				disk1,
				Overload{
					[&](const TransformedDiskColliderComponent&, entt::entity entity2)
					{
						diskDiskCollisions.emplace_back(entity1, entity2);
					},
					[&](const TransformedAABBColliderComponent&, entt::entity entity2)
					{
						diskAABBCollisions.emplace_back(entity1, entity2);
					},
					[&](const TransformedPolygonColliderComponent&, entt::entity entity2)
					{
						diskPolygonCollisions.emplace_back(entity1, entity2);
					}
				},
				[&]<typename T>(entt::entity entity2)
			{
				if constexpr (std::is_same_v<T, TransformedDiskColliderComponent>)
				{
					if (entity1 >= entity2)
					{
						return false;
					}
				}
				else
				{
					if (entity1 == entity2)
					{
						return false;
					}
				}

				const PhysicsBody2DComponent* body2 = bodyStorage.contains(entity2) ? &bodyStorage.get(entity2) : nullptr;

				return body2 != nullptr
					&& body1.mRules.GetResponse(body2->mRules) != CollisionResponse::Ignore;
			},
				BVH::DefaultShouldReturnFunction<false>{});
		}
	}

	struct CollisionResolvement
	{
		std::reference_wrapper<TransformedDiskColliderComponent> mDisk;
		glm::vec2 mDelta{};
	};
	thread_local std::vector<CollisionResolvement> collisionResolvements{};
	collisionResolvements.clear();

	auto resolveCollision = [&]<typename Shape2>(auto& collisionPairs, auto& view2)
	{
		CollisionData collision{};

		for (auto [entity1, entity2] : collisionPairs)
		{
			auto [body1, transformedDiskCollider1] = viewDisk.get<PhysicsBody2DComponent, TransformedDiskColliderComponent>(entity1);
			auto [body2, collider2] = view2.template get<PhysicsBody2DComponent, Shape2>(entity2);

			if (!CollisionCheck(transformedDiskCollider1, collider2, collision))
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
				glm::vec2 delta = ResolveDiskCollision(collision, body1, body2);
				collisionResolvements.emplace_back(transformedDiskCollider1, delta);
			}

			if constexpr (std::is_same_v<Shape2, TransformedDiskColliderComponent>)
			{
				if (body2.mIsAffectedByForces)
				{
					glm::vec2 delta = ResolveDiskCollision(collision, body2, body1, -1.0f);
					collisionResolvements.emplace_back(collider2, delta);
				}
			}
		}
	};

	resolveCollision.operator()<TransformedDiskColliderComponent>(diskDiskCollisions, viewDisk);
	resolveCollision.operator()<TransformedAABBColliderComponent>(diskAABBCollisions, viewAABB);
	resolveCollision.operator()<TransformedPolygonColliderComponent>(diskPolygonCollisions, viewPolygon);

	for (auto [disk, delta] : collisionResolvements)
	{
		disk.get().mCentre += delta;
	}

	thread_local std::vector<std::reference_wrapper<const CollisionData>> enters{};
	thread_local std::vector<std::reference_wrapper<const CollisionData>> exits{};
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

	auto view = GetWorld().GetRegistry().View<TransformedDiskColliderComponent, TransformComponent>();

	for (auto [entity, disk, transform] : view.each())
	{
		transform.SetWorldPosition(disk.mCentre);
	}

	// Call events
	CallEvents(enters, sOnCollisionEntry);
	CallEvents(currentCollisions, sOnCollisionStay);
	CallEvents(exits, sOnCollisionExit);

	std::swap(mPreviousCollisions, currentCollisions);
}

void CE::Physics::DebugDraw(RenderCommandQueue& commandQueue) const
{
	const Registry& reg = GetWorld().GetRegistry();

	if (IsDebugDrawCategoryVisible(DebugDraw::Physics))
	{
		const auto diskView = reg.View<const TransformedDiskColliderComponent, const TransformComponent>();
		constexpr glm::vec4 color = { 1.f, 0.f, 0.f, 1.f };
		for (auto [entity, disk, transform] : diskView.each())
		{
			AddDebugCircle(commandQueue, DebugDraw::Physics, To3D(disk.mCentre), disk.mRadius + 0.00001f, color);
		}

		const auto polyView = reg.View<const TransformedPolygonColliderComponent, const TransformComponent>();
		for (auto [entity, poly, transform] : polyView.each())
		{
			const size_t pointCount = poly.mPoints.size();
			for (size_t i = 0; i < pointCount; ++i)
			{
				const glm::vec2 from = poly.mPoints[i];
				const glm::vec2 to = poly.mPoints[(i + 1) % pointCount];
				AddDebugLine(commandQueue, DebugDraw::Physics, To3D(from), To3D(to), color);
			}
		}

		const auto aabbView = reg.View<const TransformedAABBColliderComponent, const TransformComponent>();
		for (auto [entity, aabb, transform] : aabbView.each())
		{
			AddDebugBox(commandQueue, DebugDraw::Physics, To3D(aabb.GetCentre()), To3D(aabb.GetSize() * .5f), color);
		}
	}

	for (const BVH& bvh : GetBVHs())
	{
		bvh.DebugDraw(commandQueue);
	}
}

CE::Physics::LineTraceResult CE::Physics::LineTrace(const Line& line, const CollisionRules& filter) const
{
	LineTraceResult result{};

	if (line.mStart == line.mEnd)
	{
		return result;
	}

	Query(
		line,
		filter,
		[&](const auto& shape, entt::entity entity)
		{
			float timeOfIntersect = CE::TimeOfLineIntersection(line, shape);
			if (timeOfIntersect < result.mDist)
			{
				result.mDist = timeOfIntersect;
				result.mHitEntity = entity;
			}
		},
		BVH::DefaultShouldCheckFunction<true>{},
		BVH::DefaultShouldReturnFunction<false>{});

	float dist = glm::distance(line.mStart, line.mEnd);
	if (result)
	{
		// We cheated and stored the time of intersect in mDist, so fix it here
		result.mDist *= dist;
	}
	else
	{
		// If we hit nothing, the distance is from start to end.
		result.mDist = dist;
	}

	return result;
}

std::vector<entt::entity> CE::Physics::FindAllWithinShape(const TransformedDisk& shape, const CollisionRules& filter) const
{
	return FindAllWithinShapeImpl(shape, filter);
}

std::vector<entt::entity> CE::Physics::FindAllWithinShape(const TransformedAABB& shape, const CollisionRules& filter) const
{
	return FindAllWithinShapeImpl(shape, filter);
}

std::vector<entt::entity> CE::Physics::FindAllWithinShape(const TransformedPolygon& shape, const CollisionRules& filter) const
{
	return FindAllWithinShapeImpl(shape, filter);
}


glm::vec2 CE::Physics::ResolveDiskCollision(const CollisionData& collisionToResolve,
	const PhysicsBody2DComponent& bodyToMove,
	const PhysicsBody2DComponent& otherBody,
	float multiplicant)
{
	// displace the objects to resolve overlap
	const float totalInvMass = bodyToMove.mInvMass + otherBody.mInvMass;
	const glm::vec2 dist = (collisionToResolve.mDepth / totalInvMass) * collisionToResolve.mNormalFor1;

	return multiplicant * dist * bodyToMove.mInvMass;
}

void CE::Physics::RegisterCollision(std::vector<CollisionData>& currentCollisions,
	CollisionData& collision, entt::entity entity1, entt::entity entity2)
{
	collision.mEntity1 = entity1;
	collision.mEntity2 = entity2;
	currentCollisions.emplace_back(collision);
}

static constexpr glm::vec2 sDefaultNormal = glm::vec2{ 0.707107f };

bool CE::Physics::CollisionCheck(TransformedDiskColliderComponent disk1, TransformedDiskColliderComponent disk2, CollisionData& result)
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

bool CE::Physics::CollisionCheck(TransformedDiskColliderComponent disk, const TransformedPolygonColliderComponent& polygon, CollisionData& result)
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

bool CE::Physics::CollisionCheck(TransformedDiskColliderComponent disk, TransformedAABBColliderComponent aabb, CollisionData& result)
{
	if (!AreOverlapping(disk, aabb))
	{
		return false;
	}

	return CollisionCheck(disk, aabb.GetAsPolygon(), result);
}

template <typename CollisionDataContainer>
void CE::Physics::CallEvents(const CollisionDataContainer& collisions,
	const EventBase& eventBase)
{
	if (collisions.empty())
	{
		return;
	}

	Registry& reg = GetWorld().GetRegistry();

	for (const BoundEvent& event : GetWorld().GetEventManager().GetBoundEvents(eventBase))
	{
		entt::sparse_set* const storage = reg.Storage(event.mType.get().GetTypeId());

		if (storage == nullptr)
		{
			continue;
		}

		for (const CollisionData& collision : collisions)
		{
			CallEvent(event, *storage, collision.mEntity1, collision.mEntity2, collision.mDepth, collision.mNormalFor1, collision.mContactPoint);
			CallEvent(event, *storage, collision.mEntity2, collision.mEntity1, collision.mDepth, -collision.mNormalFor1, collision.mContactPoint);
		}
	}
}

void CE::Physics::CallEvent(const BoundEvent& event, entt::sparse_set& storage,
	entt::entity owner, entt::entity otherEntity, float depth, glm::vec2 normal, glm::vec2 contactPoint)
{
	// Tombstone check, is needed
	if (!storage.contains(owner))
	{
		return;
	}

	if (event.mIsStatic)
	{
		event.mFunc.get().InvokeUncheckedUnpacked(GetWorld(), owner, otherEntity, depth, normal, contactPoint);
	}
	else
	{
		MetaAny component{ event.mType, storage.value(owner), false };
		event.mFunc.get().InvokeUncheckedUnpacked(component, GetWorld(), owner, otherEntity, depth, normal, contactPoint);
	}
}

CE::MetaType CE::Physics::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Physics>{}, "Physics" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const World& world, glm::vec2 centre, float radius, const CollisionRules& filter)
		{
			return world.GetPhysics().FindAllWithinShape(TransformedDisk{ centre, radius }, filter);
		}, "Find all bodies in radius", "Centre", "Radius", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const World& world, glm::vec2 min, glm::vec2 max, const CollisionRules& filter)
		{
			return world.GetPhysics().FindAllWithinShape(TransformedAABB{ min, max }, filter);
		}, "Find all bodies in box", "Min", "Max", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const World& world, glm::vec2 start, glm::vec2 end, const CollisionRules& filter)
		{
			return world.GetPhysics().LineTrace({ start, end }, filter);
		}, "LineTrace", "Start", "End", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}

CE::MetaType CE::Physics::LineTraceResult::Reflect()
{
	MetaType type = MetaType{ MetaType::T<LineTraceResult>{}, "LineTraceResult" };

	type.GetProperties()
		.Add(Props::sIsScriptableTag)
		.Add(Props::sIsScriptOwnableTag);

	type.AddField(&LineTraceResult::mDist, "mDist").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&LineTraceResult::mHitEntity, "mHitEntity").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&LineTraceResult::operator bool, "DidRayHit").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}
