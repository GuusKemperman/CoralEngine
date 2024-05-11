#include "Precomp.h"
#include "World/Physics.h"

#include "Components/TransformComponent.h"
#include "Utilities/Geometry2d.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "World/World.h"
#include "World/Registry.h"

CE::Physics::Physics(World& world) :
	mWorld(world)
{
}

CE::MetaType CE::Physics::Reflect()
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

template <typename ColliderType>
void CE::Physics::GetHeightAtPosition(glm::vec2 position, float& highestHeight) const
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

		if (height > highestHeight
			&& AreOverlapping(view.template get<const ColliderType>(entity), position))
		{
			highestHeight = height;
		}
	}
}
