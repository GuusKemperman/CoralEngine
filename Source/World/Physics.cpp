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
	mWorld(world)
{
	for (size_t i = 0; i < mBVHs.size(); i++)
	{
		mBVHs[i] = BVH{ *this, static_cast<CollisionLayer>(i) };
	}
}

CE::Physics::~Physics() = default;

template <typename T>
std::vector<entt::entity> CE::Physics::FindAllWithinShapeImpl(const T& shape, const CollisionRules& filter) const
{
	std::vector<entt::entity> ret{};

	struct OnIntersect
	{
		static void Callback(const TransformedDiskColliderComponent&, entt::entity entity, std::vector<entt::entity>& returnValue)
		{
			returnValue.emplace_back(entity);
		}

		static void Callback(const TransformedAABBColliderComponent&, entt::entity entity, std::vector<entt::entity>& returnValue)
		{
			returnValue.emplace_back(entity);
		}

		static void Callback(const TransformedPolygonColliderComponent&, entt::entity entity, std::vector<entt::entity>& returnValue)
		{
			returnValue.emplace_back(entity);
		}
	};

	for (const BVH& bvh : mBVHs)
	{
		if (filter.mResponses[static_cast<int>(bvh.GetLayer())] == CollisionResponse::Ignore)
		{
			continue;
		}

		bvh.Query<OnIntersect, BVH::DefaultShouldReturnFunction<true>, BVH::DefaultShouldReturnFunction<false>>(shape, ret);
	}

	return ret;
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
