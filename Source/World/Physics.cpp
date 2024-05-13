#include "Precomp.h"
#include "World/Physics.h"

#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/BVH.h"

CE::Physics::Physics(World& world) :
	mWorld(world),
	mBVH(std::make_unique<BVH>(*this))
{
}

CE::Physics::~Physics() = default;

template <typename T>
std::vector<entt::entity> CE::Physics::FindAllWithinShapeImpl(const T& shape, const CollisionRules& filter) const
{
	const Registry& reg = mWorld.get().GetRegistry();

	const auto diskView = reg.View<PhysicsBody2DComponent, TransformedDiskColliderComponent>();
	const auto aabbView = reg.View<PhysicsBody2DComponent, TransformedAABBColliderComponent>();
	const auto polyView = reg.View<PhysicsBody2DComponent, TransformedPolygonColliderComponent>();

	std::vector<entt::entity> returnValue{};
	returnValue.reserve(diskView.size_hint() + aabbView.size_hint() + polyView.size_hint());

	for (const entt::entity entity : diskView)
	{
		if (diskView.get<PhysicsBody2DComponent>(entity).mRules.GetResponse(filter) != CollisionResponse::Ignore
			&& AreOverlapping(diskView.get<TransformedDiskColliderComponent>(entity), shape))
		{
			returnValue.emplace_back(entity);
		}
	}

	for (const entt::entity entity : aabbView)
	{
		if (aabbView.get<PhysicsBody2DComponent>(entity).mRules.GetResponse(filter) != CollisionResponse::Ignore
			&& AreOverlapping(aabbView.get<TransformedAABBColliderComponent>(entity), shape))
		{
			returnValue.emplace_back(entity);
		}
	}

	for (const entt::entity entity : polyView)
	{
		if (polyView.get<PhysicsBody2DComponent>(entity).mRules.GetResponse(filter) != CollisionResponse::Ignore
			&& AreOverlapping(polyView.get<TransformedPolygonColliderComponent>(entity), shape))
		{
			returnValue.emplace_back(entity);
		}
	}

	return returnValue;
}

std::vector<entt::entity> CE::Physics::FindAllWithinShape(const TransformedDisk& shape, const CollisionRules& filter) const
{
	return FindAllWithinShapeImpl(shape, filter);
}

std::vector<entt::entity> CE::Physics::FindAllWithinShape(const TransformedAABB& shape, const CollisionRules& filter) const
{
	return FindAllWithinShapeImpl(shape, filter);
}

std::vector<entt::entity> CE::Physics::FindAllWithinShape(const TransformedPolygon& shape, const CollisionRules& filter) const
{
	return FindAllWithinShapeImpl(shape, filter);
}

CE::MetaType CE::Physics::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Physics>{}, "Physics" };
	type.GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](glm::vec2 centre, float radius, const CollisionRules& filter)
		{
			return World::TryGetWorldAtTopOfStack()->GetPhysics().FindAllWithinShape(TransformedDisk{ centre, radius }, filter);
		}, "Find all bodies in radius", MetaFunc::ExplicitParams<glm::vec2, float, const CollisionRules&>{}, "Centre", "Radius", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](glm::vec2 min, glm::vec2 max, const CollisionRules& filter)
		{
			return World::TryGetWorldAtTopOfStack()->GetPhysics().FindAllWithinShape(TransformedAABB{ min, max }, filter);
		}, "Find all bodies in box", MetaFunc::ExplicitParams<glm::vec2, glm::vec2, const CollisionRules&>{}, "Min", "Max", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}
