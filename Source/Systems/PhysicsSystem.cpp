#include "Precomp.h"
#include "Systems/PhysicsSystem.h"

#include "Meta/MetaType.h"
#include "World/World.h"
#include "World/Physics.h"

void CE::PhysicsSystem::Update(World& world, float dt)
{
	Physics& physics = world.GetPhysics();
	physics.Update(dt);
}

void CE::PhysicsSystem::Render(const World& world, RenderCommandQueue& commandQueue) const
{
	world.GetPhysics().DebugDraw(commandQueue);
}

CE::MetaType CE::PhysicsSystem::Reflect()
{
	return MetaType{ MetaType::T<PhysicsSystem>{}, "PhysicsSystem", MetaType::Base<System>{} };;
}
