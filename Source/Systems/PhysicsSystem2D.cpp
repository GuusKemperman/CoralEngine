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
    CheckAndRegisterCollisions(world);
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

void Engine::PhysicsSystem2D::CheckAndRegisterCollisions(World& world)
{
    Registry& reg = world.GetRegistry();
    const auto& viewDisk = reg.View<PhysicsBody2DComponent, DiskColliderComponent>();
    const auto& viewPolygon = reg.View<PhysicsBody2DComponent, PolygonColliderComponent>();

	CollisionData collision;

    for (auto [entity1, body1, disk1] : viewDisk.each())
    {
        //if (body1.GetType() == Body::Type::Static) continue;

        // disk-disk collisions
        for (auto [entity2, body2, disk2] : viewDisk.each())
        {
            if (entity1 == entity2) continue;

            // avoid duplicate collision checks
            if (/*body2.GetType() != Body::Type::Static && */entity1 > entity2) continue;

            if (CollisionCheckDiskDisk(body1.mPosition, disk1.mRadius, body2.mPosition, disk2.mRadius, collision))
            {
                //ResolveCollision(collision, body1, body2);
                RegisterCollision(collision, entity1, body1, entity2, body2);
            }
        }

        // disk-polygon collisions
        for (const auto& [entity2, body2, polygon2] : viewPolygon.each())
        {
            if (CollisionCheckDiskPolygon(body1.mPosition, disk1.mRadius, body2.mPosition, polygon2.mPoints, collision))
            {
                //ResolveCollision(collision, body1, body2);
                RegisterCollision(collision, entity1, body1, entity2, body2);
            }
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
            renderer.AddLine(DebugCategory::Physics, glm::vec2(poly.mPoints[i] + body.mPosition),
                             glm::vec2(poly.mPoints[(i + 1) % pointCount] + body.mPosition), color);
        }
        PrintCollisionData(entity, body);
    }
}

void Engine::PhysicsSystem2D::PrintCollisionData([[maybe_unused]] entt::entity entity, [[maybe_unused]] const PhysicsBody2DComponent& body)
{
    for (const auto& col : body.mCollisions)
    {
        LOG(LogPhysics, Message,
            "entity{} col - entity1: {}, entity2: {}, normal: ({}, {}), depth: {}, contact point: ({}, {})\n",
            entt::to_integral(entity), entt::to_integral(col.mEntity1), entt::to_integral(col.mEntity2), col.mNormal.x, col.mNormal.y, col.mDepth,
            col.mContactPoint.x, col.mContactPoint.y)
    }
}

void Engine::PhysicsSystem2D::RegisterCollision(CollisionData& collision, const entt::entity& entity1, PhysicsBody2DComponent& body1, const entt::entity& entity2, PhysicsBody2DComponent& body2)
{
    // store references to the entities in the CollisionData object
    collision.mEntity1 = entity1;
    collision.mEntity2 = entity2;

    // store collision data in both bodies, also if they are kinematic (implying custom collision resolution)
    // if (body1.GetType() != Body::Type::Static)
		body1.AddCollisionData(collision);
    //if (body2.GetType() != Body::Type::Static)
        body2.AddCollisionData(CollisionData{ collision.mEntity2, collision.mEntity1, -collision.mNormal, collision.mDepth });
}

bool Engine::PhysicsSystem2D::CollisionCheckDiskDisk(const glm::vec2& center1, float radius1, const glm::vec2& center2, float radius2, CollisionData& result)
{
    // check for overlap
    const glm::vec2 diff(center1 - center2);
    const float l2 = length2(diff);
    const float r = radius1 + radius2;
    if (l2 > r * r) return false;

    // compute collision details
    result.mNormal = normalize(diff);
    result.mDepth = r - sqrt(l2);
    result.mContactPoint = center2 + result.mNormal * radius2;

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
        result.mNormal = -diff / l;
        result.mDepth = l + diskRadius;
        return true;
    }

    if (l2 > diskRadius * diskRadius) return false;

    // compute collision details
    const float l = sqrt(l2);
    result.mNormal = diff / l;
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
	return MetaType{MetaType::T<PhysicsSystem2D>{}, "PhysicsSystem2D", MetaType::Base<System>{}};
}
