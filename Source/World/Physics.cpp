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

	GetHeightAtPosition<DiskColliderComponent>(position2D, highestHeight);
	GetHeightAtPosition<PolygonColliderComponent>(position2D, highestHeight);

	return highestHeight;
}

void Engine::Physics::Teleport(TransformComponent& transform, glm::vec2 toPosition) const
{
	const glm::vec3 currentPos = transform.GetWorldPosition();

	float currHeight = GetHeightAtPosition(To2DRightForward(currentPos));
	float destHeight = GetHeightAtPosition(toPosition);

	glm::vec3 targetPosition = To3DRightForward(toPosition);

	if (currHeight == -std::numeric_limits<float>::infinity())
	{
		currHeight = currentPos[Axis::Up];
	}

	if (destHeight == -std::numeric_limits<float>::infinity())
	{
		destHeight = currentPos[Axis::Up];
	}

	targetPosition[Axis::Up] = currentPos[Axis::Up] + destHeight - currHeight;
	transform.SetWorldPosition(targetPosition);
}

Engine::MetaType Engine::Physics::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Physics>{}, "Physics" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](TransformComponent& transform, glm::vec2 toPosition)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);
			world->GetPhysics().Teleport(transform, toPosition);
		}, "Teleport", MetaFunc::ExplicitParams<TransformComponent&, glm::vec2>{}, "Transform", "ToPosition").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	type.AddFunc([](glm::vec2 pos)
	{
		World* world = World::TryGetWorldAtTopOfStack();
		ASSERT(world != nullptr);
		return world->GetPhysics().GetHeightAtPosition(pos);
	}, "GetHeightAtPosition", MetaFunc::ExplicitParams<glm::vec2>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	return type;
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
		// Only colliders in the terrain layer influence the height
		if (view.template get<const PhysicsBody2DComponent>(entity).mRules.mLayer != CollisionLayer::Terrain)
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
