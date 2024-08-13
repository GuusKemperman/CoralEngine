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

		bvh.Query<OnIntersect, BVH::DefaultShouldCheckFunction<true>, BVH::DefaultShouldReturnFunction<false>>(shape, ret);
	}

	return ret;
}

void CE::Physics::RebuildBVHs(bool forceRebuild)
{
	std::array<bool, static_cast<size_t>(CollisionLayer::NUM_OF_LAYERS)> wereItemsAddedToLayer{};

	UpdateTransformedColliders<DiskColliderComponent, TransformedDiskColliderComponent>(mWorld, wereItemsAddedToLayer);
	UpdateTransformedColliders<AABBColliderComponent, TransformedAABBColliderComponent>(mWorld, wereItemsAddedToLayer);
	UpdateTransformedColliders<PolygonColliderComponent, TransformedPolygonColliderComponent>(mWorld, wereItemsAddedToLayer);

	for (int i = 0; i < static_cast<int>(CollisionLayer::NUM_OF_LAYERS); i++)
	{
		BVH& bvh = mWorld.get().GetPhysics().GetBVHs()[i];

		if (forceRebuild
			|| wereItemsAddedToLayer[i]
			|| bvh.GetAmountRefitted() > 10'000.f)
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
		}, "Find all bodies in radius", MetaFunc::ExplicitParams<const World&, glm::vec2, float, const CollisionRules&>{}, "Centre", "Radius", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	type.AddFunc([](const World& world, glm::vec2 min, glm::vec2 max, const CollisionRules& filter)
		{
			return world.GetPhysics().FindAllWithinShape(TransformedAABB{ min, max }, filter);
		}, "Find all bodies in box", MetaFunc::ExplicitParams<const World&, glm::vec2, glm::vec2, const CollisionRules&>{}, "Min", "Max", "Filter").GetProperties().Add(Props::sIsScriptableTag);

	return type;
}
