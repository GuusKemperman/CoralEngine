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
    UpdateTransforms(world, dt);
    DebugDrawing(world);
}

void Engine::PhysicsSystem2D::UpdateTransforms(World& world, float dt)
{
    Registry& reg = world.GetRegistry();
    auto view = reg.View<PhysicsBody2DComponent, TransformComponent>();
    for (auto [entity, body, transform] : view.each())
    {
        body.Update(dt);

        transform.SetLocalPosition({ body.mPosition.x, transform.GetLocalPosition().y, body.mPosition.y });
    }
}

void Engine::PhysicsSystem2D::DebugDrawing(World& world)
{
#ifdef _DEBUG
    Registry& reg = world.GetRegistry();
    const auto diskView = reg.View<DiskColliderComponent, TransformComponent>();
    const auto& renderer = world.GetRenderer();
    for (auto [entity, disk, transformComponent] : diskView.each())
    {
        renderer.AddCircle(
            DebugCategory::All, transformComponent.GetWorldPosition(), disk.mRadius + 0.1f, { 1.0f, 0.0f, 0.0f, 1.0f });
    }
    const auto polyView = reg.View<PolygonColliderComponent, TransformComponent>();
    for (auto [entity, poly, transformComponent] : polyView.each())
    {
        renderer.AddPolygon(DebugCategory::All, poly.mPoints, { 1.0f, 0.0f, 0.0f, 1.0f});
    }
#endif
}

Engine::MetaType Engine::PhysicsSystem2D::Reflect()
{
	return MetaType{MetaType::T<PhysicsSystem2D>{}, "PhysicsSystem2D", MetaType::Base<System>{}};
}
