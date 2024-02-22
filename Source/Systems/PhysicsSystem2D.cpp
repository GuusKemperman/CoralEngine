#include "Precomp.h"
#include "Systems/PhysicsSystem2D.h"

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Meta/MetaType.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"
#include "World/World.h"

void Engine::PhysicsSystem2D::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();
	auto view = reg.View<PhysicsBody2DComponent, TransformComponent>();
	for (auto [entity, body, transform] : view.each())
	{
		body.Update(dt);

		transform.SetLocalPosition({ body.mPosition.x, transform.GetLocalPosition().y, body.mPosition.y});
	}
}

Engine::MetaType Engine::PhysicsSystem2D::Reflect()
{
	return MetaType{MetaType::T<PhysicsSystem2D>{}, "PhysicsSystem2D", MetaType::Base<System>{}};
}
