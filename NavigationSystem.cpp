#include "Precomp.h"
#include "NavigationSystem.h"

#include "NavMeshAgentComponent.h"
#include "Components/TransformComponent.h"
#include "glm/glm.hpp"
#include "World/Registry.h"
#include "World/World.h"
#include "World/WorldRenderer.h"

using namespace Engine;

void NavigationSystem::Update(World& world, float dt)
{
	const auto navMeshComponentView = world.GetRegistry().View<NavMeshComponent>();

	if (world.GetRegistry().View<NavMeshComponent>().empty() == false)
	{
		for (const auto navMeshId : navMeshComponentView)
		{
			const auto& transformView = world.GetRegistry().TryGet<TransformComponent>(navMeshId);
			auto [navMeshComponent] = navMeshComponentView.get(navMeshId);

			std::vector<glm::vec3> size = {
				{navMeshComponent.m_SizeX / 2, 0, -navMeshComponent.m_SizeY / 2},
				{navMeshComponent.m_SizeX / 2, 0, navMeshComponent.m_SizeY / 2},
				{-navMeshComponent.m_SizeX / 2, 0, navMeshComponent.m_SizeY / 2},
				{-navMeshComponent.m_SizeX / 2, 0, -navMeshComponent.m_SizeY / 2}
			};

			if (transformView == nullptr)
			{
				navMeshComponent.mSize = size;
			}
			else
			{
				for (auto& i : size)
				{
					i += transformView->GetWorldPosition();
				}

				navMeshComponent.mSize = size;
			}

			if (navMeshComponent.mNavMeshNeedsUpdate)
			{
				navMeshComponent.SetNavMesh(world);
			}
		}

		// Accumulate the time to handle fixed time steps
		FixedTimeAccumulator += dt;

		while (FixedTimeAccumulator >= FixedDt)
		{
			//// Get the navmesh agent entities
			//const auto view = bee::Engine.ECS().Registry.view<NavMeshAgent, bee::Transform, Physics::Body>();

			//// There is only one entity with keyboard control
			//const auto target = bee::Engine.ECS().Registry.view<bee::KeyboardControl, bee::Transform>();

			//// Iterate over the target entity
			//for (const auto& targetId : target)
			//{
			//    auto [kc, targetT] = target.get(targetId);

			//    // Iterate over the entities that have NavMeshAgent, Transform, and Physics::Body components simultaneously
			//    for (const auto& agentId : view)
			//    {
			//        auto [n, t, b] = view.get(agentId);

			//        // Check if the agent's position is different from the target's position
			//        if (t.Translation != targetT.Translation)
			//        {
			//            // Find a path from the agent's position to the target's position
			//            n.PathFound = NavigationMesh->FindQuickestPath(t.Translation, targetT.Translation);

			//            if (!n.PathFound.empty())
			//            {
			//                // Calculate the difference in X and Y coordinates
			//                const float dx = n.PathFound[1].x - t.Translation.x;
			//                const float dy = n.PathFound[1].y - t.Translation.y;

			//                // Calculate the distance between the agent and the next waypoint
			//                const float distance = std::sqrt(dx * dx + dy * dy);

			//                if (distance > 0)
			//                {
			//                    const float step = n.GetSpeed() * dt;
			//                    if (step >= distance)
			//                    {
			//                        // If the step is larger than the remaining distance, move directly to the target position
			//                        t.Translation = glm::vec3(n.PathFound[1], 0);
			//                    }
			//                    else
			//                    {
			//                        // Calculate the new direction using linear interpolation
			//                        const float ratio = n.GetSpeed() / distance;
			//                        b.Velocity = { dx * ratio, dy * ratio };
			//                    }
			//                }
			//            }
			//        }
			//    }
			//}

			//// Deduct the fixed time step from the accumulator
			FixedTimeAccumulator -= FixedDt;
		}
	}
}

void NavigationSystem::Render(const World& world)
{
	const auto view = world.GetRegistry().View<NavMeshAgentComponent>();
	for (const auto& agentId : view)
	{
		if (const auto [n] = view.get(agentId); !n.PathFound.empty())
		{
			for (int i = 0; i < static_cast<int>(n.PathFound.size()) - 1; i++)
			{
				world.GetRenderer().AddLine(DebugCategory::AINavigation, n.PathFound[i], n.PathFound[i + 1],
				                            {1.f, 0.f, 0.f, 1.f});
				//bee::Engine.DebugRenderer().AddLine(Engine::DebugCategory::Gameplay, n.PathFound[i], n.PathFound[i + 1], { 1.f,0.f,0.f,1.f });
			}
		}
	}
	const auto view2 = world.GetRegistry().View<NavMeshComponent>();
	for (const auto& agentId : view2)
	{
		const auto [n] = view2.get(agentId);
		n.DebugDrawNavMesh(world);
	}

	// if (!PathFound.empty())
	// {
	//     for (int i = 0; i < static_cast<int>(PathFound.size())-1; i++)
	//     {
	//         bee::Engine.DebugRenderer().AddLine(bee::DebugCategory::Gameplay, PathFound[i], PathFound[i+1], {1.f,0.f,0.f,1.f});
	//     }
	// }
}

MetaType NavigationSystem::Reflect()
{
	return MetaType{MetaType::T<NavigationSystem>{}, "NavigationSystem", MetaType::Base<System>{}};
}
