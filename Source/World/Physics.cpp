#include "Precomp.h"
#include "World/Physics.h"

#include "Components/Physics2D/AABBColliderComponent.h"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/BVH.h"

CE::Physics::Physics(World& world) :
	mWorld(world),
	mBVHs{ BVH{ *this, static_cast<CollisionLayer>(0) },
		BVH{ *this, static_cast<CollisionLayer>(1) },
		BVH{ *this, static_cast<CollisionLayer>(2) },
		BVH{ *this, static_cast<CollisionLayer>(3) },
		BVH{ *this, static_cast<CollisionLayer>(4) } }
{

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

	Query<OnIntersect, BVH::DefaultShouldCheckFunction<true>, BVH::DefaultShouldReturnFunction<false>>(shape, filter, ret);

	return ret;
}

void CE::Physics::UpdateBVHs(UpdateBVHConfig config)
{
	std::array<bool, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)> wereItemsAddedToLayer{};

	UpdateTransformedColliders<DiskColliderComponent, TransformedDiskColliderComponent>(mWorld, wereItemsAddedToLayer);
	UpdateTransformedColliders<AABBColliderComponent, TransformedAABBColliderComponent>(mWorld, wereItemsAddedToLayer);
	UpdateTransformedColliders<PolygonColliderComponent, TransformedPolygonColliderComponent>(mWorld, wereItemsAddedToLayer);

	for (int i = 0; i < static_cast<int>(CollisionLayer::NUM_OF_LAYERS); i++)
	{
		BVH& bvh = mWorld.get().GetPhysics().GetBVHs()[i];

		if (config.mForceRebuild
			|| wereItemsAddedToLayer[i]
			|| (!config.mOnlyRebuildForNewColliders && bvh.GetAmountRefitted() > config.mMaxAmountRefitBeforeRebuilding))
		{
			bvh.Build();
		}
		else
		{
			bvh.Refit();
		}
	}
}

template <typename Collider, typename TransformedCollider>
void CE::Physics::UpdateTransformedColliders(World& world, std::array<bool, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)>& wereItemsAddedToLayer)
{
	Registry& reg = world.GetRegistry();
	const auto collidersWithoutTransformed = reg.View<const PhysicsBody2DComponent, const Collider>(entt::exclude_t<TransformedCollider>{});

	for (const entt::entity entity : collidersWithoutTransformed)
	{
		const PhysicsBody2DComponent& body = collidersWithoutTransformed.template get<PhysicsBody2DComponent>(entity);
		wereItemsAddedToLayer[static_cast<int>(body.mRules.mLayer)] = true;

		if (!IsCollisionLayerStatic(body.mRules.mLayer))
		{
			reg.AddComponent<TransformedCollider>(entity);
			continue;
		}

		const TransformComponent* transform = reg.TryGet<const TransformComponent>(entity);

		if (transform == nullptr)
		{
			continue;
		}

		const Collider& collider = collidersWithoutTransformed.template get<Collider>(entity);
		reg.AddComponent<TransformedCollider>(entity, collider.CreateTransformedCollider(*transform));
	}

	const auto transformedWithoutColliders = reg.View<TransformedCollider>(entt::exclude_t<Collider>{});
	reg.RemoveComponents<TransformedCollider>(transformedWithoutColliders.begin(), transformedWithoutColliders.end());

	const auto colliderView = reg.View<PhysicsBody2DComponent, TransformComponent, Collider, TransformedCollider>();

	for (auto [entity, body, transform, collider, transformedCollider] : colliderView.each())
	{
		if (IsCollisionLayerStatic(body.mRules.mLayer)
			&& world.HasBegunPlay()) // We still update static colliders in the editor
		{
			continue;
		}

		transformedCollider = collider.CreateTransformedCollider(transform);
	}
}

namespace
{
	struct OnRayIntersect
	{
		static void Callback(const auto& shape, entt::entity entity, const CE::Line& line, CE::Physics::LineTraceResult& result)
		{
			float timeOfIntersect = CE::TimeOfLineIntersection(line, shape);
			if (timeOfIntersect < result.mDist)
			{
				result.mDist = timeOfIntersect;
				result.mHitEntity = entity;
			}
		}
	};

}

CE::Physics::LineTraceResult CE::Physics::LineTrace(const Line& line, const CollisionRules& filter) const
{
	LineTraceResult result{};

	if (line.mStart == line.mEnd)
	{
		return result;
	}

	Query<OnRayIntersect,
		BVH::DefaultShouldCheckFunction<true>,
		BVH::DefaultShouldReturnFunction<false>>(line, filter, line, result);

	float dist = glm::distance(line.mStart, line.mEnd);
	if (result)
	{
		// We cheated and stored the time of intersect in mDist, so fix it here
		result.mDist *= dist;
	}
	else
	{
		// If we hit nothing, the distance is from start to end.
		result.mDist = dist;
	}

	return result;
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

	type.AddFunc([](const World& world, glm::vec2 centre, float radius, const CollisionRules& filter)
		{
			return world.GetPhysics().FindAllWithinShape(TransformedDisk{ centre, radius }, filter);
		}, "Find all bodies in radius", "Centre", "Radius", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const World& world, glm::vec2 min, glm::vec2 max, const CollisionRules& filter)
		{
			return world.GetPhysics().FindAllWithinShape(TransformedAABB{ min, max }, filter);
		}, "Find all bodies in box", "Min", "Max", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const World& world, glm::vec2 start, glm::vec2 end, const CollisionRules& filter)
		{
			return world.GetPhysics().LineTrace({ start, end }, filter);
		}, "LineTrace", "Start", "End", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}

CE::MetaType CE::Physics::LineTraceResult::Reflect()
{
	MetaType type = MetaType{ MetaType::T<LineTraceResult>{}, "LineTraceResult" };

	type.GetProperties()
		.Add(Props::sIsScriptableTag)
		.Add(Props::sIsScriptOwnableTag);

	type.AddField(&LineTraceResult::mDist, "mDist").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&LineTraceResult::mHitEntity, "mHitEntity").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc(&LineTraceResult::operator bool, "DidRayHit").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}
