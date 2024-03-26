#include "Precomp.h"
#include "Systems/NavigationSystem.h"

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshComponent.h"
#include "Components/Pathfinding/NavMeshTargetComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"

using namespace Engine;

void NavigationSystem::Update(World& world, float dt)
{
	auto& registry = world.GetRegistry();
	const auto navMeshComponentView = registry.View<NavMeshComponent>();

	for (const auto navMeshId : navMeshComponentView)
	{
		auto [navMeshComponent] = navMeshComponentView.get(navMeshId);

		// ToDo Replace mNavMesh Needs with OnConstruct
		if (navMeshComponent.mNavMeshNeedsUpdate)
		{
			navMeshComponent.GenerateNavMesh(world);
		}
	}

	if (!world.HasBegunPlay())
	{
		return;
	}

	// Get the navmesh agent entities
	const auto agentsView = registry.View<
		NavMeshAgentComponent, TransformComponent, PhysicsBody2DComponent>();

	if (navMeshComponentView.empty()) { return; }

	auto [naveMesh] = navMeshComponentView.get(navMeshComponentView.front());

	// Iterate over the entities that have NavMeshAgent, Transform, and Physics::Body components simultaneously
	for (auto [agentId, navMeshAgent, agentTransform, agentBody] : agentsView.each())
	{
		glm::vec2 agentWorldPosition = {agentTransform.GetWorldPosition().x, agentTransform.GetWorldPosition().z};
		std::optional<glm::vec2> targetPosition = navMeshAgent.GetTargetPosition();

		if (targetPosition.has_value())
		{
			const glm::vec2 toTarget = *targetPosition - agentWorldPosition;
			const float length2 = glm::length2(toTarget);

			if (length2 != 0.0f)
			{
				const glm::quat orientationQuat = Math::Direction2DToXZQuatOrientation(toTarget / glm::sqrt(length2));
				agentTransform.SetLocalOrientation(orientationQuat);
			}
		}

		if (!targetPosition.has_value() || !navMeshAgent.IsChasing())
		{
			agentBody.mLinearVelocity = {0.0f, 0.0f};
			continue;
		}

		auto characterComponent = registry.TryGet<CharacterComponent>(agentId);
		if (characterComponent == nullptr)
		{
			LOG(LogNavigationSystem, Warning, "NavMesh Agent with entity ID {} does not have a character component attached to get the movement speed.", entt::to_integral(agentId))
			continue;
		}
		float speed = characterComponent->mCurrentMovementSpeed;

		// Find a path from the agent's position to the target's position
		navMeshAgent.mPathFound = naveMesh.FindQuickestPath(agentWorldPosition,
			targetPosition.value());

		if (navMeshAgent.mPathFound.empty())
		{
			continue;
		}

		// Calculate the difference in X and Y coordinates
		const glm::vec2 dVec2 = navMeshAgent.mPathFound[1] - agentWorldPosition;

		// Calculate the distance between the agent and the next waypoint
		const float distance = glm::length(dVec2);

		if (distance == 0.0f)
		{
			continue;
		}

		if (speed * dt >= distance)
		{
			agentBody.mLinearVelocity = dVec2 / dt;
		}
		else
		{
			agentBody.mLinearVelocity = (dVec2 / distance) * speed;
		}
	}
}

void NavigationSystem::Render(const World& world)
{
	const auto agentsView = world.GetRegistry().View<NavMeshAgentComponent>();
	for (const auto [agentId, n] : agentsView.each())
	{
		if (!n.mPathFound.empty())
		{
			for (size_t i = 0; i < n.mPathFound.size() - 1; i++)
			{
				DrawDebugLine(world, DebugCategory::AINavigation, n.mPathFound[i], n.mPathFound[i + 1],
				                                 {1.f, 0.f, 0.f, 1.f});
			}
		}
	}
	const auto navMeshView = world.GetRegistry().View<NavMeshComponent>();
	for (const auto& navMeshId : navMeshView)
	{
		const auto [n] = navMeshView.get(navMeshId);
		n.DebugDrawNavMesh(world);
	}
}

MetaType NavigationSystem::Reflect()
{
	return MetaType{MetaType::T<NavigationSystem>{}, "NavigationSystem", MetaType::Base<System>{}};
}
