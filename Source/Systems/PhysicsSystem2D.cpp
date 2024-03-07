#include "Precomp.h"
#include "Systems/PhysicsSystem2D.h"

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Meta/MetaType.h"
#include "Components/TransformComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "World/WorldRenderer.h"

void Engine::PhysicsSystem2D::Update(World& world, float dt)
{
	UpdateBodiesAndTransforms(world, dt);
	UpdateCollisions(world);
#ifdef _DEBUG
	DebugDrawing(world);
#endif
}

void Engine::PhysicsSystem2D::UpdateBodiesAndTransforms(World& world, float dt)
{
	Registry& reg = world.GetRegistry();
	auto view = reg.View<PhysicsBody2DComponent, TransformComponent>();
	for (auto [entity, body, transform] : view.each())
	{
		body.Update(dt);

		transform.SetLocalPosition({ body.mPosition.x, transform.GetLocalPosition().y, body.mPosition.y });
	}
}

void Engine::PhysicsSystem2D::UpdateCollisions(World& world)
{
	Registry& reg = world.GetRegistry();

	const auto& viewDisk = reg.View<PhysicsBody2DComponent, DiskColliderComponent>();
	const auto& viewPolygon = reg.View<PhysicsBody2DComponent, PolygonColliderComponent>();

	CollisionData collision;

	// prev collisions are stored in physicssystem2d.
	static std::vector<CollisionData> currentCollisions{};
	currentCollisions.clear();

	// collisions between a dynamic and static/kinematic body are only resolved if the dynamic body is a disk
	for (auto [entity1, body1, disk1] : viewDisk.each())
	{
		if (body1.mMotionType == MotionType::Static) continue;

		// disk-disk collisions
		for (auto [entity2, body2, disk2] : viewDisk.each())
		{
			// avoid duplicate collision checks
			if (body2.mMotionType != MotionType::Static
				&& entity1 >= entity2)
			{
				continue;
			}

			if (CollisionCheckDiskDisk(body1.mPosition, disk1.mRadius, body2.mPosition, disk2.mRadius, collision))
			{
				ResolveCollision(collision, body1, body2);
				collision.mEntity1 = entity1;
				collision.mEntity2 = entity2;
				currentCollisions.emplace_back(collision);
			}
		}

		// disk-polygon collisions
		for (const auto [entity2, body2, polygon2] : viewPolygon.each())
		{
			if (CollisionCheckDiskPolygon(body1.mPosition, disk1.mRadius, body2.mPosition, polygon2.mPoints, collision))
			{
				ResolveCollision(collision, body1, body2);

				collision.mEntity1 = entity1;
				collision.mEntity2 = entity2;
				currentCollisions.emplace_back(collision);
				//RegisterCollision(collision, entity1, body1, entity2, body2);
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
		const bool isCollidingNow = std::any_of(mPreviousCollisions.begin(), mPreviousCollisions.end(),
			[&prevFrame](const CollisionData& currFrame)
			{
				return currFrame.mEntity1 == prevFrame.mEntity1
					&& currFrame.mEntity2 == prevFrame.mEntity2;
			});

		if (!isCollidingNow)
		{
			enters.emplace_back(prevFrame);
		}
	}

	// Call events
	CallEvents(world, enters, mOnCollisionEntryEvents);
	CallEvents(world, currentCollisions, mOnCollisionStayEvents);
	CallEvents(world, exits, mOnCollisionExitEvents);

	std::swap(mPreviousCollisions, currentCollisions);
}


template <typename CollisionDataContainer>
void Engine::PhysicsSystem2D::CallEvents(World& world, const CollisionDataContainer& collisions,
	const std::vector<CollisionEvent>& events)
{
	if (collisions.empty())
	{
		return;
	}

	Registry& reg = world.GetRegistry();

	for (const CollisionEvent& event : events)
	{
		entt::sparse_set* const storage = reg.Storage(event.mComponentType.get().GetTypeId());

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

void Engine::PhysicsSystem2D::DebugDrawing(World& world)
{
	Registry& reg = world.GetRegistry();
	const auto diskView = reg.View<PhysicsBody2DComponent, DiskColliderComponent, TransformComponent>();
	const auto& renderer = world.GetRenderer();
	constexpr glm::vec4 color = { 1.f, 0.f, 0.f, 1.f };
	for (auto [entity, body, disk, transformComponent] : diskView.each())
	{
		renderer.AddCircle(
			DebugCategory::Physics, transformComponent.GetWorldPosition(), disk.mRadius + 0.1f, color);
		PrintCollisionData(entity, body);
	}
	const auto polyView = reg.View<PhysicsBody2DComponent, PolygonColliderComponent, TransformComponent>();
	for (auto [entity, body, poly, transformComponent] : polyView.each())
	{
		const size_t pointCount = poly.mPoints.size();
		for (size_t i = 0; i < pointCount; ++i)
		{
			const glm::vec2 from = poly.mPoints[i] + body.mPosition;
			const glm::vec2 to = poly.mPoints[(i + 1) % pointCount] + body.mPosition;
			renderer.AddLine(DebugCategory::Physics, glm::vec3(from.x, 1.1f, from.y),
				glm::vec3(to.x, 1.1f, to.y), color);
		}
	}
}

void Engine::PhysicsSystem2D::CallEvent(const CollisionEvent& event, World& world, entt::sparse_set& storage,
	entt::entity owner, entt::entity otherEntity, float depth, glm::vec2 normal, glm::vec2 contactPoint)
{
	// Tombstone check, is needed
	if (!storage.contains(owner))
	{
		return;
	}

	if (event.mIsStatic)
	{
		// TODO pass arguments
		event.mEvent.get().InvokeUncheckedUnpacked();
	}
	else
	{
		// TODO pass arguments
		event.mEvent.get().InvokeUncheckedUnpacked();
	}
}

void Engine::PhysicsSystem2D::ResolveCollision(const CollisionData& collision, PhysicsBody2DComponent& body1, PhysicsBody2DComponent& body2)
{
	const bool isBody1Dynamic = body1.mMotionType == MotionType::Dynamic;
	const bool isBody2Dynamic = body2.mMotionType == MotionType::Dynamic;

	// if both bodies are not dynamic, there's nothing left to do
	if (!isBody1Dynamic && !isBody2Dynamic) return;

	// displace the objects to resolve overlap
	const float totalInvMass = body1.mInvMass + body2.mInvMass;
	const glm::vec2& dist = (collision.mDepth / totalInvMass) * collision.mNormalFor1;

	if (isBody1Dynamic) body1.mPosition += dist * body1.mInvMass;
	if (isBody2Dynamic) body2.mPosition += dist * body2.mInvMass;

	// compute and apply impulses
	const float dotProduct = glm::dot(body1.mLinearVelocity - body2.mLinearVelocity, collision.mNormalFor1);
	if (dotProduct <= 0)
	{
		const float restitution = std::min(body1.mRestitution, body2.mRestitution);
		const float j = -(1 + restitution) * dotProduct / totalInvMass;
		const glm::vec2& impulse = j * collision.mNormalFor1;

		if (isBody1Dynamic) body1.ApplyImpulse(impulse);
		if (isBody2Dynamic) body2.ApplyImpulse(-impulse);
	}
}

//void Engine::PhysicsSystem2D::RegisterCollision(CollisionData& collision, const entt::entity& entity1, PhysicsBody2DComponent& body1, const entt::entity& entity2, PhysicsBody2DComponent& body2)
//{
//	// store collision data in both bodies, also if they are kinematic (implying custom collision resolution)
//	if (body1.mMotionType != MotionType::Static)
//	{
//		collision.mOtherEntity = entity2;
//		body1.AddCollisionData(collision);
//	}
//
//	if (body2.mMotionType != MotionType::Static)
//	{
//		collision.mOtherEntity = entity1;
//		collision.mNormalFor1 = -collision.mNormalFor1;
//		body2.AddCollisionData(collision);
//	}
//}

bool Engine::PhysicsSystem2D::CollisionCheckDiskDisk(const glm::vec2& center1, float radius1, const glm::vec2& center2, float radius2, CollisionData& result)
{
	// check for overlap
	const glm::vec2 diff(center1 - center2);
	const float l2 = length2(diff);
	const float r = radius1 + radius2;
	if (l2 > r * r) return false;

	// compute collision details
	result.mNormalFor1 = normalize(diff);
	result.mDepth = r - sqrt(l2);
	result.mContactPoint = center2 + result.mNormalFor1 * radius2;

	return true;
}

bool Engine::PhysicsSystem2D::CollisionCheckDiskPolygon(const glm::vec2& diskCenter, float diskRadius, const glm::vec2& polygonPos, const std::vector<glm::vec2>& polygonPoints,
	CollisionData& result)
{
	const glm::vec2& nearest = GetNearestPointOnPolygonBoundary(diskCenter - polygonPos, polygonPoints) + polygonPos;
	const glm::vec2 diff(diskCenter - nearest);
	const float l2 = length2(diff);

	if (IsPointInsidePolygon(diskCenter - polygonPos, polygonPoints))
	{
		const float l = sqrt(l2);
		result.mNormalFor1 = -diff / l;
		result.mDepth = l + diskRadius;
		return true;
	}

	if (l2 > diskRadius * diskRadius) return false;

	// compute collision details
	const float l = sqrt(l2);
	result.mNormalFor1 = diff / l;
	result.mDepth = diskRadius - l;
	result.mContactPoint = nearest;
	return true;
}

bool Engine::PhysicsSystem2D::IsPointInsidePolygon(const glm::vec2& point, const std::vector<glm::vec2>& polygon)
{
	// Adapted from: https://wrfranklin.org/Research/Short_Notes/pnpoly.html

	size_t i, j;
	const size_t n = polygon.size();
	bool inside = false;

	for (i = 0, j = n - 1; i < n; j = i++)
	{
		if ((polygon[i].y > point.y != polygon[j].y > point.y) &&
			(point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x))
			inside = !inside;
	}

	return inside;
}

glm::vec2 Engine::PhysicsSystem2D::GetNearestPointOnPolygonBoundary(const glm::vec2& point, const std::vector<glm::vec2>& polygon)
{
	float bestDist = std::numeric_limits<float>::max();
	glm::vec2 bestNearest(0.f, 0.f);

	const size_t n = polygon.size();
	for (size_t i = 0; i < n; ++i)
	{
		const glm::vec2& nearest = GetNearestPointOnLineSegment(point, polygon[i], polygon[(i + 1) % n]);
		const float dist = distance2(point, nearest);
		if (dist < bestDist)
		{
			bestDist = dist;
			bestNearest = nearest;
		}
	}

	return bestNearest;
}

glm::vec2 Engine::PhysicsSystem2D::GetNearestPointOnLineSegment(const glm::vec2& p, const glm::vec2& segmentA, const glm::vec2& segmentB)
{
	const float t = dot(p - segmentA, segmentB - segmentA) / distance2(segmentA, segmentB);
	if (t <= 0) return segmentA;
	if (t >= 1) return segmentB;
	return (1 - t) * segmentA + t * segmentB;
}

Engine::MetaType Engine::PhysicsSystem2D::Reflect()
{
	return MetaType{ MetaType::T<PhysicsSystem2D>{}, "PhysicsSystem2D", MetaType::Base<System>{} };
}
