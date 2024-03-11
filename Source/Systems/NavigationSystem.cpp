#include "Precomp.h"
#include "Systems/NavigationSystem.h"

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/TransformComponent.h"
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
	const auto navMeshComponentView = world.GetRegistry().View<NavMeshComponent>();

	for (const auto navMeshId : navMeshComponentView)
	{
		auto [navMeshComponent] = navMeshComponentView.get(navMeshId);

		// ToDo Replace mNavMeshNeeds with OnConstruct
		if (navMeshComponent.mNavMeshNeedsUpdate)
		{
			navMeshComponent.SetNavMesh(world);
		}
	}

	//// Get the navmesh agent entities
	const auto agentsView = world.GetRegistry().View<
		NavMeshAgentComponent, TransformComponent, PhysicsBody2DComponent>();

	//// There is only one entity with keyboard control
	const auto targetsView = world.GetRegistry().View<NavMeshTargetTag, TransformComponent>();

	if (navMeshComponentView.empty()) { return; }

	auto [naveMesh] = navMeshComponentView.get(navMeshComponentView.front());

	//// Iterate over the target entity
	for (auto [targetId, targetTransform] : targetsView.each())
	{
		auto targetWorldPosition = targetTransform.GetWorldPosition();

		// Iterate over the entities that have NavMeshAgent, Transform, and Physics::Body components simultaneously
		for (auto [agentId, navMeshAgent, agentTransform, agentBody] : agentsView.each())
		{
			auto agentWorldPosition = agentTransform.GetWorldPosition();

			// Check if the agent's position is different from the target's position
			if (agentWorldPosition != targetWorldPosition)
			{
				// Find a path from the agent's position to the target's position
				navMeshAgent.mPathFound = naveMesh.FindQuickestPath(
					{agentWorldPosition.x, agentWorldPosition.z},
					{targetWorldPosition.x, targetWorldPosition.z});

				if (!navMeshAgent.mPathFound.empty())
				{
					// Calculate the difference in X and Y coordinates
					const glm::vec2 dVec2 = navMeshAgent.mPathFound[1] - glm::vec2{
						agentWorldPosition.x, agentWorldPosition.z
					};

					// Calculate the distance between the agent and the next waypoint
					const float distance = std::sqrt(dVec2.x * dVec2.x + dVec2.y * dVec2.y);

					if (distance > 0)
					{
						const float step = navMeshAgent.GetSpeed() * dt;
						if (step >= distance)
						{
							// If the step is larger than the remaining distance, move directly to the target position
							agentTransform.SetWorldPosition(
								glm::vec3(navMeshAgent.mPathFound[1].x, 0, navMeshAgent.mPathFound[1].y));
						}
						else
						{
							// Calculate the new direction using linear interpolation
							const float ratio = navMeshAgent.GetSpeed() / distance;
							agentBody.mLinearVelocity = ratio * dVec2;
						}
					}
				}
			}
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
				world.GetDebugRenderer().AddLine(DebugCategory::AINavigation, n.mPathFound[i], n.mPathFound[i + 1],
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
