#include "Precomp.h"
#include "Systems/SwarmingSystem.h"

#include <queue>

#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Components/Pathfinding/SwarmingAgentTag.h"
#include "Components/Pathfinding/SwarmingTargetComponent.h"
#include "Meta/MetaType.h"
#include "Rendering/DebugRenderer.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/SteeringBehaviours.h"
#include "World/Physics.h"
#include "World/Registry.h"

void CE::SwarmingAgentSystem::Update(World& world, float)
{
	Registry& reg = world.GetRegistry();

	const auto targetView = reg.View<SwarmingTargetComponent, TransformComponent>();
	const entt::entity targetEntity = targetView.front();

	if (targetEntity == entt::null)
	{
		return;
	}

	const auto& [target, targetTransform] = targetView.get(targetEntity);
	const glm::vec2 targetPos = targetTransform.GetWorldPosition2D();
	const float interpolationFactor = 1.0f / (target.mSpacing * target.mSpacing);

	for (auto [entity, transform, character, body, collider] : reg.View<SwarmingAgentTag, TransformComponent, CharacterComponent, PhysicsBody2DComponent, TransformedDiskColliderComponent>().each())
	{
		const glm::vec2 position = transform.GetWorldPosition2D();
		const glm::vec2 positionInGridSpace = position - target.mCellsTopLeftWorldPosition;
		const int cellX = static_cast<int>(positionInGridSpace.x / target.mSpacing);
		const int cellY = static_cast<int>(positionInGridSpace.y / target.mSpacing);

		const glm::vec2 toTarget = targetPos - position;
		const float dist2ToTarget = glm::length2(toTarget);

		if (dist2ToTarget == 0.0f)
		{
			continue;
		}

		glm::vec2 flowFieldDir{};

		if (cellX < 0
			|| cellY < 0
			|| cellX >= target.mFlowFieldWidth
			|| cellY >= target.mFlowFieldWidth
			|| dist2ToTarget < target.mSpacing * target.mSpacing)
		{
			flowFieldDir = toTarget / glm::sqrt(dist2ToTarget);
		}
		else
		{
			// Some bilinear interpolation
			const uint32 sampleIndex = cellX + cellY * target.mFlowFieldWidth;
			const bool onEdgeX = cellX + 1 >= target.mFlowFieldWidth;
			const bool onEdgeZ = cellY + 1 >= target.mFlowFieldWidth;

			const glm::vec2 sampleX0Z0 = target.mFlowField[sampleIndex];
			const glm::vec2 sampleX1Z0 = onEdgeX ? sampleX0Z0 : target.mFlowField[sampleIndex + 1];
			const glm::vec2 sampleX0Z1 = onEdgeZ ? sampleX0Z0 : target.mFlowField[sampleIndex + target.mFlowFieldWidth];
			const glm::vec2 sampleX1Z1 = onEdgeX || onEdgeZ ? sampleX0Z0 : target.mFlowField[sampleIndex + target.mFlowFieldWidth + 1];

			const float weightX = fmodf(positionInGridSpace.x, target.mSpacing);
			const float weightZ = fmodf(positionInGridSpace.y, target.mSpacing);
			const float opposingWeightX = target.mSpacing - weightX;
			const float opposingWeightY = target.mSpacing - weightZ;

			DrawDebugLine(world, DebugCategory::AINavigation, target.GetCellBox(cellX, cellY).GetCentre(), target.GetCellBox(cellX, cellY).GetCentre() + sampleX0Z0 * opposingWeightX * opposingWeightY, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
			DrawDebugLine(world, DebugCategory::AINavigation, target.GetCellBox(cellX + 1, cellY).GetCentre(), target.GetCellBox(cellX + 1, cellY).GetCentre() + sampleX1Z0 * weightX * opposingWeightY, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
			DrawDebugLine(world, DebugCategory::AINavigation, target.GetCellBox(cellX, cellY + 1).GetCentre(), target.GetCellBox(cellX, cellY + 1).GetCentre() + sampleX0Z1 * opposingWeightX * weightZ, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
			DrawDebugLine(world, DebugCategory::AINavigation, target.GetCellBox(cellX + 1, cellY + 1).GetCentre(), target.GetCellBox(cellX + 1, cellY + 1).GetCentre() + sampleX1Z1 * weightX * weightZ, glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });

			DrawDebugRectangle(world, DebugCategory::AINavigation, To3DRightForward(target.GetCellBox(cellX, cellY).GetCentre(), 0.0f), target.GetCellBox(cellX, cellY).GetSize() * .5f, glm::vec4{ 1.0f });
			DrawDebugRectangle(world, DebugCategory::AINavigation, To3DRightForward(target.GetCellBox(cellX + 1, cellY).GetCentre(), 0.0f), target.GetCellBox(cellX, cellY).GetSize() * .5f, glm::vec4{ 0.5f });
			DrawDebugRectangle(world, DebugCategory::AINavigation, To3DRightForward(target.GetCellBox(cellX, cellY + 1).GetCentre(), 0.0f), target.GetCellBox(cellX, cellY).GetSize() * .5f, glm::vec4{ 0.5f });
			DrawDebugRectangle(world, DebugCategory::AINavigation, To3DRightForward(target.GetCellBox(cellX + 1, cellY + 1).GetCentre(), 0.0f), target.GetCellBox(cellX, cellY).GetSize() * .5f, glm::vec4{ .25f });

			flowFieldDir = interpolationFactor * (
				sampleX0Z0 * opposingWeightX * opposingWeightY +
				sampleX1Z0 * weightX * opposingWeightY +
				sampleX0Z1 * opposingWeightX * weightZ +
				sampleX1Z1 * weightX * weightZ
				);
		}

		const glm::vec2 avoidanceDir = CalculateAvoidanceVelocity(world, entity, collider.mRadius * 2.0f, transform, collider);

		const glm::vec2 desiredDir = CombineVelocities(avoidanceDir, flowFieldDir);

		const float length2 = glm::length2(desiredDir);

		if (length2 != 0.0f)
		{
			transform.SetWorldOrientation(Math::Direction2DToXZQuatOrientation(desiredDir / glm::sqrt(length2)));
		}

		body.mLinearVelocity = desiredDir * character.mCurrentMovementSpeed;
	}
}

void CE::SwarmingTargetSystem::Update(World& world, float)
{
	Registry& reg = world.GetRegistry();
	const BVH& bvh = world.GetPhysics().GetBVHs()[static_cast<int>(CollisionLayer::StaticObstacles)];

	float minRadius = std::numeric_limits<float>::infinity();
	const auto agentView = reg.View<SwarmingAgentTag, TransformedDiskColliderComponent>();

	for (auto [entity, disk] : agentView.each())
	{
		minRadius = std::min(disk.mRadius, minRadius);
	}

	for (auto [entity, target, disk] : reg.View<SwarmingTargetComponent, TransformedDiskColliderComponent>().each())
	{
		const glm::vec2 targetPosition = disk.mCentre;
		target.mSpacing = std::min(disk.mRadius, minRadius);

		int fieldWidth = std::max(static_cast<int>((target.mDesiredRadius * 2.0f) / target.mSpacing), 3);

		if ((fieldWidth & 1) == 0)
		{
			fieldWidth++;
		}

		const float fieldWidthWorldSpace = static_cast<float>(fieldWidth) * target.mSpacing;

		target.mCellsTopLeftWorldPosition = targetPosition - glm::vec2{ fieldWidthWorldSpace } *.5f;
		target.mFlowFieldWidth = fieldWidth;
		target.mFlowField.resize(static_cast<size_t>(fieldWidth * fieldWidth));

		// A floodfill does not cover enclosed areas.
		// We initialize all cells with the direction
		// to the player
		for (int y = 0; y < target.mFlowFieldWidth; y++)
		{
			for (int x = 0; x < target.mFlowFieldWidth; x++)
			{
				const glm::vec2 cellCentre = target.GetCellBox(x, y).GetCentre();
				glm::vec2 toTarget = targetPosition - cellCentre;

				if (toTarget == glm::vec2{})
				{
					toTarget = glm::vec2{ 1.0f, 0.0f };
				}

				target.mFlowField[x + y * fieldWidth] = glm::normalize(toTarget);
			}
		}

		// char because std::vector<bool> is slower
		std::vector<char> isBlocked(target.mFlowField.size(), false);

		for (int y = 0; y < target.mFlowFieldWidth; y++)
		{
			for (int x = 0; x < target.mFlowFieldWidth; x++)
			{
				const TransformedAABB cell = target.GetCellBox(x, y);
				isBlocked[x + y * target.mFlowFieldWidth] = bvh.Query<BVH::DefaultOnIntersectFunction, BVH::DefaultShouldReturnFunction<true>, BVH::DefaultShouldReturnFunction<true>>(cell);
			}
		}

		const int centre = fieldWidth / 2;
		const int startIndex = centre + centre * fieldWidth;

		std::queue<int> openIndices{};
		openIndices.emplace(startIndex);

		std::vector distanceField(target.mFlowField.size(), std::numeric_limits<float>::infinity());
		distanceField[startIndex] = 0.0f;

		while (!openIndices.empty())
		{
			const int current = openIndices.front();
			openIndices.pop();

			const float currentDist = distanceField[current];

			const int x = current % fieldWidth;
			const int y = current / fieldWidth;

			const auto exploreNbr = [&](const int nbrX, const int nbrY, const float distIncease)
				{
					if (nbrY < 0
						|| nbrY >= fieldWidth
						|| nbrX < 0
						|| nbrX >= fieldWidth)
					{
						return;
					}

					const int nbrIndex = nbrX + nbrY * fieldWidth;

					if (isBlocked.data()[nbrIndex])
					{
						return;
					}

					const float nbrDist = currentDist + distIncease;

					if (nbrDist < distanceField[nbrIndex])
					{
						distanceField[nbrIndex] = nbrDist;
						openIndices.emplace(nbrIndex);
					}
				};

			exploreNbr(x - 1, y - 1, 1.41421357f);
			exploreNbr(x + 1, y + 1, 1.41421357f);
			exploreNbr(x + 1, y - 1, 1.41421357f);
			exploreNbr(x - 1, y + 1, 1.41421357f);

			exploreNbr(x - 1, y, 1.0f);
			exploreNbr(x, y - 1, 1.0f);
			exploreNbr(x + 1, y, 1.0f);
			exploreNbr(x, y + 1, 1.0f);
		}

		for (int y = 0; y < fieldWidth; y++)
		{
			for (int x = 0; x < fieldWidth; x++)
			{
				float lowestDist = std::numeric_limits<float>::infinity();

				// Captured bindings...
				SwarmingTargetComponent& targetRef = target;

				const auto checkNbr = [&](const int nbrX, const int nbrY)
					{
						if (nbrY < 0
							|| nbrY >= fieldWidth
							|| nbrX < 0
							|| nbrX >= fieldWidth)
						{
							return;
						}

						const int nbrIndex = nbrX + nbrY * fieldWidth;
						const float dist = distanceField[nbrIndex];

						if ((fabsf(lowestDist - dist) < 0.5f
							&& ((nbrX & 1) || (nbrY & 1)))
							|| dist < lowestDist)
						{
							lowestDist = dist;
							targetRef.mFlowField[x + y * fieldWidth] = glm::normalize(glm::vec2{ static_cast<float>(nbrX - x), static_cast<float>(nbrY - y) });
						}
					};

				checkNbr(x - 1, y - 1);
				checkNbr(x + 1, y + 1);
				checkNbr(x + 1, y - 1);
				checkNbr(x - 1, y + 1);

				checkNbr(x - 1, y);
				checkNbr(x, y - 1);
				checkNbr(x + 1, y);
				checkNbr(x, y + 1);
			}
		}
	}
}

void CE::SwarmingTargetSystem::Render(const World& world)
{
	if (!DebugRenderer::IsCategoryVisible(DebugCategory::AINavigation))
	{
		return;
	}

	for (auto [entity, target] : world.GetRegistry().View<SwarmingTargetComponent>().each())
	{
		for (int y = 0; y < target.mFlowFieldWidth; y++)
		{
			for (int x = 0; x < target.mFlowFieldWidth; x++)
			{
				const TransformedAABB cell = target.GetCellBox(x, y);

				const glm::vec2 pos = cell.GetCentre();
				const glm::vec2 dir = target.mFlowField[x + y * target.mFlowFieldWidth];

				DrawDebugLine(world, DebugCategory::AINavigation, pos, pos + dir, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f });
			}
		}
	}
}

CE::MetaType CE::SwarmingAgentSystem::Reflect()
{
	return MetaType{ MetaType::T<SwarmingAgentSystem>{}, "SwarmingAgentSystem", MetaType::Base<System>{} };
}

CE::MetaType CE::SwarmingTargetSystem::Reflect()
{
	return MetaType{ MetaType::T<SwarmingTargetSystem>{}, "SwarmingTargetSystem", MetaType::Base<System>{} };
}
