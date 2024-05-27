#include "Precomp.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::SwarmingAgentTag::StartMovingToTarget(World& world, entt::entity entity)
{
	if (!world.GetRegistry().HasComponent<SwarmingAgentTag>(entity))
	{
		world.GetRegistry().AddComponent<SwarmingAgentTag>(entity);
	}
}

void CE::SwarmingAgentTag::StopMovingToTarget(World& world, entt::entity entity)
{
	if (!world.GetRegistry().HasComponent<SwarmingAgentTag>(entity))
	{
		return;
	}

	world.GetRegistry().RemoveComponent<SwarmingAgentTag>(entity);

	PhysicsBody2DComponent* const body = world.GetRegistry().TryGet<PhysicsBody2DComponent>(entity);

	if (body != nullptr)
	{
		body->mLinearVelocity = glm::vec2{};
	}
}

CE::MetaType CE::SwarmingAgentTag::Reflect()
{
	MetaType type{ MetaType::T<SwarmingAgentTag>{}, "SwarmingAgentTag" };

	ReflectComponentType<SwarmingAgentTag>(type);
	return type;
}
