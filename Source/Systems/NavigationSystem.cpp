#include "Precomp.h"
#include "Systems/NavigationSystem.h"

#include <intsafe.h>

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Pathfinding/NavMeshAgentComponent.h"
#include "Components/Pathfinding/NavMeshComponent.h"
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
		const CharacterComponent& character, 
		const PhysicsBody2DComponent& body,
		const TransformedDiskColliderComponent& characterCollider)
	{
		const glm::vec2 myWorldPos = characterCollider.mCentre;

		TransformedDisk avoidanceDisk = characterCollider;
		avoidanceDisk.mRadius += character.mAvoidanceDistance * transform.GetWorldScaleUniform2D();
		avoidanceDisk.mRadius *= 2.0f;

		std::vector<entt::entity> collidedWith = world.GetPhysics().FindAllWithinShape(avoidanceDisk, body.mRules);

		glm::vec2 avoidanceVelocity{};

		for (const entt::entity obstacle : collidedWith)
		{
			// Ignore myself
			if (obstacle == self)
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

		std::optional<glm::vec2> targetPosition = agent.GetTargetPosition(world);

		if (!targetPosition.has_value())
		{
			return {};
		}

		const glm::vec2 agentWorldPosition = transform.GetWorldPosition2D();

		const float speed = character.mCurrentMovementSpeed;
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
			return {};
		}

		const float howFarWeCanMove = speed * dt;

		if (distance > howFarWeCanMove)
		{
			return dVec2 / howFarWeCanMove;
		}
		return glm::normalize(dVec2);
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
			Internal::CalculateAvoidanceVelocity(world, entity, transform, characterComponent, body, transformedDisk),
			Internal::CalculatePathFollowingVelocity(world, transform, characterComponent, navMeshComponent, dt));

		const float length2 = glm::length2(desiredVel);

		if (length2 != 0.0f)
		{
			const glm::quat orientationQuat = Math::Direction2DToXZQuatOrientation(desiredVel);
			transform.SetLocalOrientation(orientationQuat);
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
