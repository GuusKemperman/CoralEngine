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
#include "Utilities/DebugRenderer.h"

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
			navMeshComponent.SetNavMesh(world);
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

		// set orientation
		if (targetPosition.has_value())
		{
			const glm::vec2 orientation2D = glm::normalize(targetPosition.value() - agentWorldPosition);
			const glm::quat orientationQuat = Math::Direction2DToXZQuatOrientation(orientation2D);
			agentTransform.SetLocalOrientation(orientationQuat);
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
		                                                    navMeshAgent.GetTargetPosition().value());

		if (!navMeshAgent.mPathFound.empty())
		{
			// Calculate the difference in X and Y coordinates
			const glm::vec2 dVec2 = navMeshAgent.mPathFound[1] - agentWorldPosition;

			// Calculate the distance between the agent and the next waypoint
			const float distance = std::sqrt(dVec2.x * dVec2.x + dVec2.y * dVec2.y);

			if (distance > 0)
			{
				const float step = speed * dt;
				if (step >= distance)
				{
					// If the step is larger than the remaining distance, move directly to the target position
					agentTransform.SetWorldPosition(
						glm::vec3(navMeshAgent.mPathFound[1].x, 0, navMeshAgent.mPathFound[1].y));
				}
				else
				{
					// Calculate the new direction using linear interpolation
					const float ratio = speed / distance;
					agentBody.mLinearVelocity = ratio * dVec2;
				}
			}
		}
	}
	//}
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
				world.GetDebugRenderer().AddLine(world, DebugCategory::AINavigation, n.mPathFound[i], n.mPathFound[i + 1],
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
