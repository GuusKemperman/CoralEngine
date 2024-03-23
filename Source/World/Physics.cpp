#include "Precomp.h"
#include "World/Physics.h"

#include "Components/TransformComponent.h"
#include "Components/Pathfinding/Geometry2d.hpp"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "World/World.h"
#include "World/Registry.h"

Engine::Physics::Physics(World& world) :
	mWorld(world)
{
}

float Engine::Physics::GetHeightAtPosition(glm::vec2 position2D) const
{
	float highestHeight = -std::numeric_limits<float>::infinity();

	static_assert(sNumOfDifferentColliderTypes == 2,
		"This is one more place you need to account for the new collider type."
		"Increment the number after ==, after you finish you accounting for the new collider.");

	GetHeightAtPosition<DiskColliderComponent>(position2D, highestHeight);
	GetHeightAtPosition<PolygonColliderComponent>(position2D, highestHeight);

	return highestHeight;
}

bool Engine::Physics::IsPointInsideCollider(glm::vec2 point, glm::vec2 colliderPos,
	const DiskColliderComponent& diskCollider)
{
	return IsPointInsideDisk(point, colliderPos, diskCollider.mRadius);
}

bool Engine::Physics::IsPointInsideCollider(glm::vec2 point, glm::vec2 worldPos,
	const PolygonColliderComponent& polygonCollider)
{
	point -= worldPos;
	return IsPointInsidePolygon(point, polygonCollider.mPoints);
}

template <typename ColliderType>
void Engine::Physics::GetHeightAtPosition(glm::vec2 position, float& highestHeight) const
{
	const Registry& reg = mWorld.get().GetRegistry();

	auto view = reg.View<const TransformComponent, const PhysicsBody2DComponent, const ColliderType>();

	// Don't do .each, as we may be able to do an early out, preventing us from
	// having to retrieve the other components at all.
	for (entt::entity entity : view)
	{
		if (view.template get<const PhysicsBody2DComponent>(entity).mRules.mLayer != CollisionLayer::WorldStatic)
		{
			continue;
		}

		const glm::vec3 worldPos = view.template get<const TransformComponent>(entity).GetWorldPosition();
		const float height = worldPos[Axis::Up];
		const glm::vec2 worldPos2D = To2DRightForward(worldPos);

		if (height > highestHeight
			&& IsPointInsideCollider(position, worldPos2D, view.template get<const ColliderType>(entity)))
		{
			highestHeight = height;

		}
	}
}
