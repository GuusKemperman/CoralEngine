#include "Precomp.h"
#include "Systems/PhysicsSystem2D.h"

#include "Components/ComponentFilter.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Meta/MetaType.h"
#include "Components/TransformComponent.h"
#include "Components/Pathfinding/Geometry2d.hpp"
#include "Components/Physics2D/DiskColliderComponent.h"
#include "Components/Physics2D/PolygonColliderComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "World/WorldViewport.h"
#include "World/Physics.h"
#include "Rendering/DrawDebugHelpers.h"

Engine::PhysicsSystem2D::PhysicsSystem2D()
{
	for (const MetaType& type : MetaManager::Get().EachType())
	{
		if (!ComponentFilter::IsTypeValid(type))
		{
			continue;
		}

		const MetaFunc* const entry = TryGetEvent(type, sCollisionEntryEvent);
		const MetaFunc* const stay = TryGetEvent(type, sCollisionStayEvent);
		const MetaFunc* const exit = TryGetEvent(type, sCollisionExitEvent);

		if (entry != nullptr)
		{
			mOnCollisionEntryEvents.emplace_back(CollisionEvent{ type, *entry, entry->GetProperties().Has(Props::sIsEventStaticTag) });
		}

		if (stay != nullptr)
		{
			mOnCollisionStayEvents.emplace_back(CollisionEvent{ type, *stay, stay->GetProperties().Has(Props::sIsEventStaticTag) });
		}

		if (exit != nullptr)
		{
			mOnCollisionExitEvents.emplace_back(CollisionEvent{ type, *exit, exit->GetProperties().Has(Props::sIsEventStaticTag) });
		}
	}
}

void Engine::PhysicsSystem2D::Update(World& world, float dt)
{
	UpdateBodiesAndTransforms(world, dt);
	UpdateCollisions(world);
}

void Engine::PhysicsSystem2D::Render(const World& world)
{
	DebugDrawing(world);
}

void Engine::PhysicsSystem2D::UpdateBodiesAndTransforms(World& world, float dt)
{
	Registry& reg = world.GetRegistry();
	const Physics& physics = world.GetPhysics();

	const auto view = reg.View<PhysicsBody2DComponent, TransformComponent>();
	for (auto [entity, body, transform] : view.each())
	{
		if (body.mIsAffectedByForces)
		{
			body.mLinearVelocity += body.mForce * body.mInvMass * dt;
		}

		if (body.mLinearVelocity == glm::vec2{})
		{
			continue;
		}

		const glm::vec2 translation = body.mLinearVelocity * dt;
		const glm::vec3 oldPos = transform.GetWorldPosition();
		const glm::vec3 newPos = GetAllowedWorldPos(physics, body, oldPos, translation);

		body.mLinearVelocity = To2DRightForward(newPos - oldPos) / dt;
		transform.SetWorldPosition(newPos);
	}
}

void Engine::PhysicsSystem2D::UpdateCollisions(World& world)
{
	Registry& reg = world.GetRegistry();
	const Physics& physics = world.GetPhysics();

	const auto& viewDisk = reg.View<PhysicsBody2DComponent, TransformComponent, DiskColliderComponent>();
	const auto& viewPolygon = reg.View<PhysicsBody2DComponent, TransformComponent, PolygonColliderComponent>();

	CollisionData collision;

	// prev collisions are stored as a member function,
	// but this is made static so we can reuse the same
	// buffer every frame, even if we have multiple worlds.
	static std::vector<CollisionData> currentCollisions{};
	currentCollisions.clear();

	// collisions between a dynamic and static/kinematic body are only resolved if the dynamic body is a disk
	for (auto [entity1, body1, transform1, disk1] : viewDisk.each())
	{
		// Can be modified by ResolveCollision. Is only actually applied
		// to the transform at the end of this loop, for performance
		// reasons.
		glm::vec3 entity1Pos = transform1.GetWorldPosition();
		const glm::vec3 entity1PosAtStart = entity1Pos;

		// disk-disk collisions
		for (auto [entity2, body2, transform2, disk2] : viewDisk.each())
		{
			// Avoid duplicate collision checks
			if (entity1 >= entity2)
			{
				continue;
			}

			const CollisionResponse response = body1.mRules.GetResponse(body2.mRules);

			if (response == CollisionResponse::Ignore)
			{
				continue;
			}

			const glm::vec3 entity2Pos = transform2.GetWorldPosition();

			if (CollisionCheckDiskDisk(To2DRightForward(entity1Pos), disk1.mRadius, To2DRightForward(entity2Pos), disk2.mRadius, collision))
			{
				RegisterCollision(currentCollisions, collision, entity1, entity2);

				if (response == CollisionResponse::Blocking)
				{
					ResolveCollision(physics, collision, body1, body2, transform2, entity1Pos, entity2Pos);
				}
			}
		}

		// disk-polygon collisions
		for (const auto [entity2, body2, transform2, polygon2] : viewPolygon.each())
		{
			const CollisionResponse response = body1.mRules.GetResponse(body2.mRules);

			if (response == CollisionResponse::Ignore)
			{
				continue;
			}

			const glm::vec3 entity2Pos = transform2.GetWorldPosition();

			if (CollisionCheckDiskPolygon(To2DRightForward(entity1Pos), disk1.mRadius, To2DRightForward(entity2Pos), polygon2.mPoints, collision))
			{
				RegisterCollision(currentCollisions, collision, entity1, entity2);

				if (response == CollisionResponse::Blocking)
				{
					ResolveCollision(physics, collision, body1, body2, transform2, entity1Pos, entity2Pos);
				}
			}
		}

		if (entity1Pos != entity1PosAtStart)
		{
			transform1.SetWorldPosition(GetAllowedWorldPos(physics, body1, entity1PosAtStart, To2DRightForward(entity1Pos - entity1PosAtStart)));
		}
	}

	static std::vector<std::reference_wrapper<const CollisionData>> enters{};
	static std::vector<std::reference_wrapper<const CollisionData>> exits{};
	enters.clear();
	exits.clear();

	for (const CollisionData& currFrame : currentCollisions)
	{
		const bool wasCollidingPrevFrame = std::any_of(mPreviousCollisions.begin(), mPreviousCollisions.end(),
		                                               [&currFrame](const CollisionData& prevFrame)
		                                               {
			                                               return currFrame.mEntity1 == prevFrame.mEntity1
				                                               && currFrame.mEntity2 == prevFrame.mEntity2;
		                                               });

		if (!wasCollidingPrevFrame)
		{
			enters.emplace_back(currFrame);
		}
	}

	for (const CollisionData& prevFrame : mPreviousCollisions)
	{
		const bool isCollidingNow = std::any_of(currentCollisions.begin(), currentCollisions.end(),
		                                        [&prevFrame](const CollisionData& currFrame)
		                                        {
			                                        return currFrame.mEntity1 == prevFrame.mEntity1
				                                        && currFrame.mEntity2 == prevFrame.mEntity2;
		                                        });

		if (!isCollidingNow)
		{
			exits.emplace_back(prevFrame);
		}
	}

	// Call events
	CallEvents(world, enters, mOnCollisionEntryEvents);
	CallEvents(world, currentCollisions, mOnCollisionStayEvents);
	CallEvents(world, exits, mOnCollisionExitEvents);

	std::swap(mPreviousCollisions, currentCollisions);
}

template <typename CollisionDataContainer>
void Engine::PhysicsSystem2D::CallEvents(World& world, const CollisionDataContainer& collisions,
                                         const std::vector<CollisionEvent>& events)
{
	if (collisions.empty())
	{
		return;
	}

	Registry& reg = world.GetRegistry();

	for (const CollisionEvent& event : events)
	{
		entt::sparse_set* const storage = reg.Storage(event.mComponentType.get().GetTypeId());

		if (storage == nullptr)
		{
			continue;
		}

		for (const CollisionData& collision : collisions)
		{
			CallEvent(event, world, *storage, collision.mEntity1, collision.mEntity2, collision.mDepth, collision.mNormalFor1, collision.mContactPoint);
			CallEvent(event, world, *storage, collision.mEntity2, collision.mEntity1, collision.mDepth, -collision.mNormalFor1, collision.mContactPoint);
		}
	}
}

void Engine::PhysicsSystem2D::DebugDrawing(const World& world)
{
	const Registry& reg = world.GetRegistry();

	if (DebugRenderer::GetDebugCategoryFlags() & DebugCategory::Physics)
	{
		const auto diskView = reg.View<const DiskColliderComponent, const TransformComponent>();
		constexpr glm::vec4 color = { 1.f, 0.f, 0.f, 1.f };
		for (auto [entity, disk, transformComponent] : diskView.each())
		{
			DrawDebugCircle(world, DebugCategory::Physics, transformComponent.GetWorldPosition(), disk.mRadius + 0.1f, color);
		}

		const auto polyView = reg.View<const PolygonColliderComponent, const TransformComponent>();
		for (auto [entity, poly, transformComponent] : polyView.each())
		{
			const glm::vec2 worldPos = transformComponent.GetWorldPosition2D();
			const size_t pointCount = poly.mPoints.size();
			for (size_t i = 0; i < pointCount; ++i)
			{
				const glm::vec2 from = poly.mPoints[i] + worldPos;
				const glm::vec2 to = poly.mPoints[(i + 1) % pointCount] + worldPos;
				DrawDebugLine(world, DebugCategory::Physics, glm::vec3(from.x, 1.1f, from.y), glm::vec3(to.x, 1.1f, to.y), color);
			}
		}
	}

	if (DebugRenderer::GetDebugCategoryFlags() & DebugCategory::TerrainHeight)
	{
		const Physics& physics = world.GetPhysics();

		auto optCamera = world.GetViewport().GetMainCamera();

		if (!optCamera.has_value())
		{
			return;
		}

		const TransformComponent* const cameraTransform = world.GetRegistry().TryGet<const TransformComponent>(optCamera->first);

		if (cameraTransform == nullptr)
		{
			return;
		}

		const glm::vec3 cameraWorldPos = cameraTransform->GetWorldPosition();
		const glm::vec2 cameraPos2D = To2DRightForward(cameraWorldPos);

		constexpr float range = 50.0f;
		constexpr float spacing = 5.0f;
		for (float x = -range; x < range; x += spacing)
		{
			for (float y = -range; y < range; y += spacing)
			{
				glm::vec2 worldPos2D = cameraPos2D + glm::vec2{ x, y };
				worldPos2D.x -= fmodf(cameraPos2D.x + x, spacing);
				worldPos2D.y -= fmodf(cameraPos2D.y + y, spacing);

				const float height = physics.GetHeightAtPosition(worldPos2D);

				if (height == -std::numeric_limits<float>::infinity())
				{
					continue;
				}

				const float maxExpectedHeight = cameraWorldPos[Axis::Up] + range * .5f;
				const float minExpectedHeight = cameraWorldPos[Axis::Up] - range * .5f;
				const float colorT = Math::lerpInv(minExpectedHeight, maxExpectedHeight,
						glm::clamp(height, minExpectedHeight, maxExpectedHeight));

				constexpr glm::vec4 minColor = { 0.0f, 0.0f, 5.0f, 1.0f };
				constexpr glm::vec4 maxColor = { 5.0f, 0.0f, 0.0f, 1.0f };
				const glm::vec4 color = Math::lerp(minColor, maxColor, colorT);

				DrawDebugSquare(world, DebugCategory::TerrainHeight, To3DRightForward(worldPos2D, height), spacing, color);
			}
		}
	}
}

void Engine::PhysicsSystem2D::CallEvent(const CollisionEvent& event, World& world, entt::sparse_set& storage,
        entt::entity owner, entt::entity otherEntity, float depth, glm::vec2 normal, glm::vec2 contactPoint)
{
	static_assert(std::is_same_v<decltype(sCollisionEntryEvent), const Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)>>);
	static_assert(std::is_same_v<decltype(sCollisionStayEvent), const Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)>>);
	static_assert(std::is_same_v<decltype(sCollisionExitEvent), const Event<void(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)>>);

	// Tombstone check, is needed
	if (!storage.contains(owner))
	{
		return;
	}

	if (event.mIsStatic)
	{
		event.mEvent.get().InvokeUncheckedUnpacked(world, owner, otherEntity, depth, normal, contactPoint);
	}
	else
	{
		MetaAny component{ event.mComponentType, storage.value(owner), false };
		event.mEvent.get().InvokeUncheckedUnpacked(component, world, owner, otherEntity, depth, normal, contactPoint);
	}
}

void Engine::PhysicsSystem2D::ResolveCollision(const Physics& physics, 
		const CollisionData& collision,
		PhysicsBody2DComponent& body1,
		PhysicsBody2DComponent& body2,
		TransformComponent& transform2,
		glm::vec3& entity1WorldPos,
		glm::vec3 entity2WorldPos)
{
	const bool isBody1Dynamic = body1.mIsAffectedByForces;
	const bool isBody2Dynamic = body2.mIsAffectedByForces;

	if (isBody1Dynamic 
		|| isBody2Dynamic)
	{
		// displace the objects to resolve overlap
		const float totalInvMass = body1.mInvMass + body2.mInvMass;
		const glm::vec2 dist = (collision.mDepth / totalInvMass) * collision.mNormalFor1;

		if (isBody1Dynamic)
		{
			// entity1WorldPos is applied to the transform
			// outside of the loop that iterates over all the entity2's,
			// see UpdateCollisions.
			entity1WorldPos = GetAllowedWorldPos(physics, body1, entity1WorldPos, dist * body1.mInvMass);
		}

		if (isBody2Dynamic)
		{
			transform2.SetWorldPosition(GetAllowedWorldPos(physics, body2, entity2WorldPos, -dist * body2.mInvMass));
		}

		// compute and apply impulses
		const float dotProduct = dot(body1.mLinearVelocity - body2.mLinearVelocity, collision.mNormalFor1);

		if (dotProduct <= 0)
		{
			const float restitution = (body1.mRestitution + body2.mRestitution);
			const float j = -(1 + restitution * 0.5f) * dotProduct / (1 / body1.mInvMass + 1 / body2.mInvMass);
			const glm::vec2 impulse = j * collision.mNormalFor1;

			if (isBody1Dynamic)
			{
				body1.ApplyImpulse(impulse);
			}

			if (isBody2Dynamic)
			{
				body2.ApplyImpulse(-impulse);
			}
		}
	}
}

void Engine::PhysicsSystem2D::RegisterCollision(std::vector<CollisionData>& currentCollisions,
		CollisionData& collision, entt::entity entity1, entt::entity entity2)
{
	collision.mEntity1 = entity1;
	collision.mEntity2 = entity2;
	currentCollisions.emplace_back(collision);
}


bool Engine::PhysicsSystem2D::CollisionCheckDiskDisk(glm::vec2 center1, float radius1, glm::vec2 center2,
		float radius2, CollisionData& result)
{
	// check for overlap
	const glm::vec2 diff(center1 - center2);
	const float l2 = length2(diff);
	const float r = radius1 + radius2;
	if (l2 > r * r) return false;

	// compute collision details
	result.mNormalFor1 = normalize(diff);
	result.mDepth = r - sqrt(l2);
	result.mContactPoint = center2 + result.mNormalFor1 * radius2;

	return true;
}

bool Engine::PhysicsSystem2D::CollisionCheckDiskPolygon(glm::vec2 diskCenter, float diskRadius, glm::vec2 polygonPos, const std::vector<glm::vec2>& polygonPoints,
		CollisionData& result)
{
	glm::vec2 nearest = GetNearestPointOnPolygonBoundary(diskCenter - polygonPos, polygonPoints) + polygonPos;
	const glm::vec2 diff(diskCenter - nearest);
	const float l2 = length2(diff);

	if (IsPointInsidePolygon(diskCenter - polygonPos, polygonPoints))
	{
		const float l = sqrt(l2);
		result.mNormalFor1 = -diff / l;
		result.mDepth = l + diskRadius;
		return true;
	}

	if (l2 > diskRadius * diskRadius) return false;

	// compute collision details
	const float l = sqrt(l2);
	result.mNormalFor1 = diff / l;
	result.mDepth = diskRadius - l;
	result.mContactPoint = nearest;
	return true;
}

glm::vec3 Engine::PhysicsSystem2D::GetAllowedWorldPos(const Physics& physics, 
	const PhysicsBody2DComponent& body, 
	glm::vec3 currentWorldPos,
	glm::vec2 translation)
{
	const glm::vec2 currentWorldPos2D = To2DRightForward(currentWorldPos);
	const glm::vec2 desiredWorldPos2D = currentWorldPos2D + translation;

	if (body.mRules.GetResponseIncludingTerrain(CollisionPresets::sTerrain.mRules) != CollisionResponse::Blocking)
	{
		return To3DRightForward(desiredWorldPos2D, currentWorldPos[Axis::Up]);
	}

	const float heightAtCurrPos = physics.GetHeightAtPosition(currentWorldPos2D);
	const float heightAtDesiredPos = physics.GetHeightAtPosition(desiredWorldPos2D);
	const float dtHeight = heightAtDesiredPos - heightAtCurrPos;

	if (Physics::IsHeightDifferenceTraversable(dtHeight))
	{
		return To3DRightForward(desiredWorldPos2D, currentWorldPos[Axis::Up] + dtHeight);
	}
	return currentWorldPos;
}

Engine::MetaType Engine::PhysicsSystem2D::Reflect()
{
	return MetaType{ MetaType::T<PhysicsSystem2D>{}, "PhysicsSystem2D", MetaType::Base<System>{} };
}
