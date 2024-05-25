#include "Precomp.h"
#include "Utilities/SteeringBehaviours.h"

#include "Components/TransformComponent.h"
#include "Components/Pathfinding/NavMeshTargetTag.h"
#include "Components/Pathfinding/SwarmingTargetComponent.h"
#include "Utilities/BVH.h"
#include "World/Physics.h"

glm::vec2 CE::CombineVelocities(const glm::vec2 dominantVelocity, const glm::vec2 recessiveVelocity)
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

glm::vec2 CE::CalculateAvoidanceVelocity(const World& world, const entt::entity self, const float avoidanceRadius,
	const TransformComponent& characterTransform, const TransformedDisk& characterCollider)
{
	const glm::vec2 myWorldPos = characterCollider.mCentre;
	TransformedDisk avoidanceDisk = characterCollider;
	avoidanceDisk.mRadius += (avoidanceRadius * 2.0f) * characterTransform.GetWorldScaleUniform2D();

	const std::vector<entt::entity> collidedWith = world.GetPhysics().FindAllWithinShape(  avoidanceDisk, 
		{
			CollisionLayer::Character,
			{
				CollisionResponse::Ignore,		//WorldStatic
				CollisionResponse::Ignore,		// WorldDynamic
				CollisionResponse::Blocking,	// Character
				CollisionResponse::Ignore,		// Terrain
				CollisionResponse::Ignore,		// Query
			}
		});

	glm::vec2 avoidanceVelocity{};

	for (const entt::entity obstacle : collidedWith)
	{
		if (obstacle == self // Ignore myself
			|| world.GetRegistry().HasComponent<NavMeshTargetTag>(obstacle) // Do not avoid the target
			|| world.GetRegistry().HasComponent<SwarmingTargetComponent>(obstacle))
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
		avoidanceVelocity = glm::normalize(avoidanceVelocity);
	}

	return avoidanceVelocity;
}

