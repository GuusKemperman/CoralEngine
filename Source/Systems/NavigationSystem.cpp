#include "Precomp.h"
#include "Systems/NavigationSystem.h"

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"

void CE::NavigationSystem::Update(World& world, float dt)
{
	Registry& registry = world.GetRegistry();

	const entt::entity navMeshOwner = registry.View<NavMeshComponent>().front();

	if (navMeshOwner == entt::null)
	{
		return;
	}

	NavMeshComponent& navMesh = registry.Get<NavMeshComponent>(navMeshOwner);

	if(!navMesh.WasGenerated())
	{
		navMesh.GenerateNavMesh(world);
	}

	// Get the navmesh agent entities
	const auto agentsView = registry.View<NavMeshAgentComponent, TransformComponent, PhysicsBody2DComponent>();

	for (auto [agentId, agent, agentTransform, agentBody] : agentsView.each())
	{
		if (!agent.IsChasing())
		{
			continue;
		}

		std::optional<glm::vec2> targetPosition = agent.GetTargetPosition(world);

		if (!targetPosition.has_value())
		{
			continue;
		}

		const glm::vec2 agentWorldPosition = agentTransform.GetWorldPosition2D();
		const glm::vec2 toTarget = *targetPosition - agentWorldPosition;
		const float length2 = glm::length2(toTarget);

		if (length2 != 0.0f)
		{
			const glm::quat orientationQuat = Math::Direction2DToXZQuatOrientation(toTarget / glm::sqrt(length2));
			agentTransform.SetLocalOrientation(orientationQuat);
		}

		auto characterComponent = registry.TryGet<CharacterComponent>(agentId);
		if (characterComponent == nullptr)
		{
			LOG(LogNavigationSystem, Warning,
				"NavMesh Agent with entity ID {} does not have a character component attached to get the movement speed.",
				entt::to_integral(agentId));
			continue;
		}
		const float speed = characterComponent->mCurrentMovementSpeed;
		const float desiredDistToPoint2 = Math::sqr(speed * .1f);

		agent.mPath.erase(agent.mPath.begin(), std::find_if(agent.mPath.begin(), agent.mPath.end(),
			[desiredDistToPoint2, agentWorldPosition](const glm::vec2& point)
			{
				return glm::distance2(agentWorldPosition, point) > desiredDistToPoint2;
			}));

		const glm::vec2 moveTowards = agent.mPath.empty() ? *targetPosition : agent.mPath[0];

		const glm::vec2 dVec2 = moveTowards - agentWorldPosition;

		const float distance = length(dVec2);

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

void CE::NavigationSystem::Render(const World& world)
{
	if (!DebugRenderer::IsCategoryVisible(DebugCategory::AINavigation))
	{
		return;
	}

	const auto agentsView = world.GetRegistry().View<NavMeshAgentComponent>();
	for (const auto [agentId, n] : agentsView.each())
	{
		if (!n.mPath.empty())
		{
			for (size_t i = 0; i < n.mPath.size() - 1; i++)
			{
				DrawDebugLine(
					world,
					DebugCategory::AINavigation,
					To3DRightForward(n.mPath[i]),
					To3DRightForward(n.mPath[i + 1]),
					{ 1.f, 0.f, 0.f, 1.f });
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

CE::MetaType CE::NavigationSystem::Reflect()
{
	return MetaType{MetaType::T<NavigationSystem>{}, "NavigationSystem", MetaType::Base<System>{}};
}

void CE::UpdatePathsSystem::Update(World& world, float)
{
	mNumOfTicksReceived++;

	Registry& registry = world.GetRegistry();

	const entt::entity navMeshOwner = registry.View<NavMeshComponent>().front();

	if (navMeshOwner == entt::null)
	{
		return;
	}

	NavMeshComponent& navMesh = registry.Get<NavMeshComponent>(navMeshOwner);

	if (!navMesh.WasGenerated())
	{
		return;
	}

	const auto agentsView = registry.View<NavMeshAgentComponent, TransformComponent>();
	uint32 index{};

	for (auto [agentId, agent, agentTransform] : agentsView.each())
	{
		if ((mNumOfTicksReceived + index++) % sUpdateEveryNthPath
			|| !agent.IsChasing())
		{
			continue;
		}

		if (!agent.IsChasing())
		{
			continue;
		}

		std::optional<glm::vec2> targetPosition = agent.GetTargetPosition(world);

		if (!targetPosition.has_value())
		{
			continue;
		}

		agent.mPath = navMesh.FindQuickestPath(agentTransform.GetWorldPosition2D(), *targetPosition);
	}
}

CE::MetaType CE::UpdatePathsSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdatePathsSystem>{}, "UpdatePathsSystem", MetaType::Base<System>{} };
}
