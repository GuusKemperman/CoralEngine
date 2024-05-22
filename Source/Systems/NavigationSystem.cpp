#include "Precomp.h"
#include "Systems/NavigationSystem.h"

#include <intsafe.h>

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"
#include "Utilities/DrawDebugHelpers.h"
#include "World/Physics.h"

namespace CE::Internal
{
	static glm::vec2 CombineVelocities(const glm::vec2 dominantVelocity, const glm::vec2 recessiveVelocity)
	{
		const float oldRecessiveLength = glm::length(recessiveVelocity);

		if (oldRecessiveLength == 0.0f) // Prevents divide by zero
		{
			return dominantVelocity;
		}

		// Floating point errors require the min..
		const float dominantLength = std::min(glm::length(dominantVelocity), 1.0f);

		const float newRecessiveLength = std::min(1.0f - dominantLength, oldRecessiveLength);
		const glm::vec2 newRecessive = (recessiveVelocity / oldRecessiveLength) * newRecessiveLength;

		return dominantVelocity + newRecessive;
	}

	static glm::vec2 CalculateAvoidanceVelocity(const World& world,
		const entt::entity self,
		const TransformComponent& transform,
		const NavMeshAgentComponent& agent,
		const PhysicsBody2DComponent& body,
		const TransformedDiskColliderComponent& characterCollider)
	{
		const glm::vec2 myWorldPos = characterCollider.mCentre;

		TransformedDisk avoidanceDisk = characterCollider;
		avoidanceDisk.mRadius += agent.mAvoidanceDistance * transform.GetWorldScaleUniform2D();
		avoidanceDisk.mRadius *= 2.0f;

		std::vector<entt::entity> collidedWith = world.GetPhysics().FindAllWithinShape(avoidanceDisk, body.mRules);

		glm::vec2 avoidanceVelocity{};

		for (const entt::entity obstacle : collidedWith)
		{
			if (obstacle == self // Ignore myself
				|| world.GetRegistry().HasComponent<NavMeshTargetTag>(obstacle)) // Do not avoid the target
			{
				continue;
			}

			const TransformComponent* obstacleTransform = world.GetRegistry().TryGet<TransformComponent>(obstacle);
			if (obstacleTransform == nullptr)
			{
				continue;
			}

			const glm::vec2 obstaclePosition2D = obstacleTransform->GetWorldPosition2D();
			glm::vec2 deltaPos = myWorldPos - obstaclePosition2D;

			float deltaPosLength = length(deltaPos);

			// Prevents division by 0
			if (deltaPosLength == 0.0f)
			{
				deltaPos = glm::vec2{ 0.01f };
				deltaPosLength = length(deltaPos);
			}

			const float avoidanceStrength = std::clamp((1.0f - (deltaPosLength / avoidanceDisk.mRadius)), 0.0f, 1.0f);

			avoidanceVelocity += (deltaPos / deltaPosLength) * avoidanceStrength;
		}

		if (glm::length2(avoidanceVelocity) > 1.0f)
		{
			avoidanceVelocity = normalize(avoidanceVelocity);
		}

		return avoidanceVelocity;
	}

	static glm::vec2 CalculatePathFollowingVelocity(const World& world,
		const TransformComponent& transform,
		const CharacterComponent& character,
		NavMeshAgentComponent& agent,
		float dt)
	{
		if (!agent.IsChasing())
		{
			return {};
		}

		std::optional<glm::vec2> destination = agent.GetTargetPosition(world);

		if (!destination.has_value())
		{
			return {};
		}

		const glm::vec2 agentWorldPosition = transform.GetWorldPosition2D();
		glm::vec2 targetPoint = *destination;

		if (!agent.mPath.empty())
		{
			size_t bestIndex{};

			for (size_t i = 1; i < agent.mPath.size() - 1; i++)
			{
				Line line{ agent.mPath[i], agent.mPath[i + 1] };

				DrawDebugLine(
					world,
					DebugCategory::AINavigation,
					To3DRightForward(line.mStart),
					To3DRightForward(line.mEnd),
					{ 1.f, 1.f, 1.f, 1.f });

				if (line.SignedDistance(agentWorldPosition) <= Line{ agent.mPath[bestIndex], agent.mPath[bestIndex + 1] }.SignedDistance(agentWorldPosition))
				{
					bestIndex = i;
				}
			}

			float distLeft = agent.mAdditionalDistanceAlongPathToTarget;
			targetPoint = Line{ agent.mPath[bestIndex], agent.mPath[bestIndex + 1] }.ClosestPointOnLine(agentWorldPosition);

			for (size_t i = bestIndex; i < agent.mPath.size() - 1 && distLeft > 0.0f; i++)
			{
				Line line{ targetPoint, agent.mPath[i + 1] };

				const float lineLength = glm::distance(line.mStart, line.mEnd);

				if (lineLength == 0.0f)
				{
					continue;
				}

				const glm::vec2 dir = (line.mEnd - line.mStart) / lineLength;

				const float distToAdd = glm::min(lineLength, distLeft);
				distLeft -= distToAdd;
				targetPoint += dir * distToAdd;
			}
		}

		DrawDebugLine(world, DebugCategory::AINavigation, agentWorldPosition, targetPoint, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });

		const float howFarWeCanMove = character.mCurrentMovementSpeed * dt;
		const glm::vec2 toTarget = targetPoint - agentWorldPosition;
		const float distToTarget = glm::length(toTarget);

		// Dont overshoot
		if (distToTarget > howFarWeCanMove)
		{
			return toTarget / howFarWeCanMove;
		}

		if (distToTarget == 0.0f)
		{
			return {};
		}

		return toTarget / distToTarget;
	}
}

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
	const auto agentsView = registry.View<NavMeshAgentComponent, CharacterComponent, TransformComponent, PhysicsBody2DComponent, TransformedDiskColliderComponent>();


	for (auto [entity, navMeshComponent, characterComponent, transform, body, transformedDisk] : agentsView.each())
	{
		const glm::vec2 desiredVel = Internal::CombineVelocities(
			Internal::CalculateAvoidanceVelocity(world, entity, transform, navMeshComponent, body, transformedDisk),
			Internal::CalculatePathFollowingVelocity(world, transform, characterComponent, navMeshComponent, dt));

		const float length2 = glm::length2(desiredVel);

		if (length2 != 0.0f)
		{
			transform.SetWorldOrientation(Math::Direction2DToXZQuatOrientation(desiredVel / glm::sqrt(length2)));
		}

		body.mLinearVelocity = desiredVel * characterComponent.mCurrentMovementSpeed;
	}
}

void CE::NavigationSystem::Render(const World& world)
{
	if (!DebugRenderer::IsCategoryVisible(DebugCategory::AINavigation))
	{
		return;
	}

	for (const auto [entity, agent] : world.GetRegistry().View<NavMeshAgentComponent>().each())
	{
		if (agent.mPath.empty())
		{
			continue;
		}

		for (size_t i = 0; i < agent.mPath.size() - 1; i++)
		{
			DrawDebugLine(world, DebugCategory::AINavigation, agent.mPath[i], agent.mPath[i + 1], glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f });
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
