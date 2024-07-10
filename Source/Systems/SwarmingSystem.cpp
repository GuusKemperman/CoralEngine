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

	const BVH& bvh = world.GetPhysics().GetBVHs()[static_cast<int>(CollisionLayer::StaticObstacles)];

	const SwarmingTargetComponent& target = targetView.get<SwarmingTargetComponent>(targetEntity);
	const TransformComponent& targetTransform = targetView.get<TransformComponent>(targetEntity);
	const glm::vec2 targetPos = targetTransform.GetWorldPosition2D();
	const float interpolationFactor = 1.0f / (target.mCurrent.mSpacing * target.mCurrent.mSpacing);

	for (auto [entity, transform, character, body, collider] : reg.View<SwarmingAgentTag, TransformComponent, CharacterComponent, PhysicsBody2DComponent, TransformedDiskColliderComponent>().each())
	{
		const glm::vec2 agentPosition = transform.GetWorldPosition2D();

		const glm::vec2 avoidanceDir = CalculateAvoidanceVelocity(world, entity, collider.mRadius * 2.0f, transform, collider);

		const glm::vec2 toTarget = targetPos - agentPosition;
		glm::vec2 desiredDirectionTowardsTarget{};

		const float dist2ToTarget = glm::length2(toTarget);

		if (dist2ToTarget != 0.0f)
		{
			float distToTarget = glm::sqrt(dist2ToTarget);
			glm::vec2 toTargetDir = toTarget / distToTarget;

			const glm::vec2 offset1 = glm::vec2{ -toTargetDir.y, toTargetDir.x } * collider.mRadius;
			const glm::vec2 offset2 = -offset1;

			const Line line1{ agentPosition, targetPos };
			const Line line2{ agentPosition + offset1, targetPos + offset1 };
			const Line line3{ agentPosition + offset2, targetPos + offset2 };

			DrawDebugLine(world, DebugCategory::AINavigation, line1.mStart, line1.mEnd, glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f });
			DrawDebugLine(world, DebugCategory::AINavigation, line2.mStart, line2.mEnd, glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f });
			DrawDebugLine(world, DebugCategory::AINavigation, line3.mStart, line3.mEnd, glm::vec4{ 0.0f, 1.0f, 1.0f, 1.0f });

			// Check if we can see the target
			if (bvh.Query(line1)
				|| bvh.Query(line2)
				|| bvh.Query(line3))
			{
				const auto getDirectionSample = [&](const glm::vec2 samplePosition) -> glm::vec2
					{
						const glm::vec2 positionInGridSpace = samplePosition - target.mCurrent.mCellsTopLeftWorldPosition;
						const int cellX = static_cast<int>(positionInGridSpace.x / target.mCurrent.mSpacing);
						const int cellY = static_cast<int>(positionInGridSpace.y / target.mCurrent.mSpacing);

						if (cellX < 0
							|| cellY < 0
							|| cellX >= target.mCurrent.mFlowFieldWidth
							|| cellY >= target.mCurrent.mFlowFieldWidth
							|| dist2ToTarget < target.mCurrent.mSpacing * target.mCurrent.mSpacing)
						{
							return toTarget / glm::sqrt(dist2ToTarget);
						}

						// Some bilinear interpolation
						const uint32 sampleIndex = cellX + cellY * target.mCurrent.mFlowFieldWidth;
						const bool onEdgeX = cellX + 1 >= target.mCurrent.mFlowFieldWidth;
						const bool onEdgeZ = cellY + 1 >= target.mCurrent.mFlowFieldWidth;

						const glm::vec2 sampleX0Z0 = target.mCurrent.mFlowField[sampleIndex];
						const glm::vec2 sampleX1Z0 = onEdgeX ? sampleX0Z0 : target.mCurrent.mFlowField[sampleIndex + 1];
						const glm::vec2 sampleX0Z1 = onEdgeZ ? sampleX0Z0 : target.mCurrent.mFlowField[sampleIndex + target.mCurrent.mFlowFieldWidth];
						const glm::vec2 sampleX1Z1 = onEdgeX || onEdgeZ ? sampleX0Z0 : target.mCurrent.mFlowField[sampleIndex + target.mCurrent.mFlowFieldWidth + 1];

						const float weightX = fmodf(positionInGridSpace.x, target.mCurrent.mSpacing);
						const float weightZ = fmodf(positionInGridSpace.y, target.mCurrent.mSpacing);
						const float opposingWeightX = target.mCurrent.mSpacing - weightX;
						const float opposingWeightY = target.mCurrent.mSpacing - weightZ;

						return interpolationFactor * (
							sampleX0Z0 * opposingWeightX * opposingWeightY +
							sampleX1Z0 * weightX * opposingWeightY +
							sampleX0Z1 * opposingWeightX * weightZ +
							sampleX1Z1 * weightX * weightZ
							);
					};

				const float sampleSpacing = collider.mRadius;
				desiredDirectionTowardsTarget =
					(getDirectionSample(agentPosition) * 2.0f
						+ getDirectionSample({ agentPosition.x - sampleSpacing, agentPosition.y })
						+ getDirectionSample({ agentPosition.x + sampleSpacing, agentPosition.y })
						+ getDirectionSample({ agentPosition.x, agentPosition.y + sampleSpacing })
						+ getDirectionSample({ agentPosition.x, agentPosition.y - sampleSpacing })
						* (1.0f / 6.0f));
			}
			else
			{
				desiredDirectionTowardsTarget = toTargetDir;
			}
		}

		const glm::vec2 desiredDir = CombineVelocities(avoidanceDir, desiredDirectionTowardsTarget);

		const float length2 = glm::length2(desiredDir);

		if (length2 != 0.0f)
		{
			transform.SetWorldOrientation(Math::Direction2DToXZQuatOrientation(desiredDir / glm::sqrt(length2)));
		}

		body.mLinearVelocity = desiredDir * character.mCurrentMovementSpeed;
	}
}

CE::SwarmingTargetSystem::~SwarmingTargetSystem()
{
	if (mPendingThread.joinable())
	{
		mPendingThread.join();
	}
}

void CE::SwarmingTargetSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();
	const auto targetView = reg.View<SwarmingTargetComponent, TransformedDiskColliderComponent>();

	const entt::entity targetEntity = targetView.front();

	if (targetEntity == entt::null)
	{
		return;
	}

	SwarmingTargetComponent& target = targetView.get<SwarmingTargetComponent>(targetEntity);

	if (mPendingThread.joinable()
		&& mIsPendingReady)
	{
		mPendingThread.join();
		mIsPendingReady = false;

		if (mPendingForEntity == targetEntity)
		{
			std::swap(target.mCurrent, mPendingFlowField);
		}
	}

	if (!mStartNewThreadCooldown.IsReady(dt)
		|| mPendingThread.joinable())
	{
		return;
	}

	mPendingForEntity = targetEntity;

	const BVH& bvh = world.GetPhysics().GetBVHs()[static_cast<int>(CollisionLayer::StaticObstacles)];

	float minRadius = std::numeric_limits<float>::infinity();
	const auto agentView = reg.View<SwarmingAgentTag, TransformedDiskColliderComponent>();

	for (auto [entity, disk] : agentView.each())
	{
		minRadius = std::min(disk.mRadius, minRadius);
	}

	const TransformedDiskColliderComponent& targetDisk = targetView.get<TransformedDiskColliderComponent>(targetEntity);
	const glm::vec2 targetPosition = targetDisk.mCentre;
	mPendingFlowField.mSpacing = minRadius;
	mPendingFlowField.mFlowFieldWidth = std::max(static_cast<int>((target.mDesiredRadius * 2.0f) / mPendingFlowField.mSpacing), 3);

	if ((mPendingFlowField.mFlowFieldWidth & 1) == 0)
	{
		mPendingFlowField.mFlowFieldWidth++;
	}

	const float fieldWidthWorldSpace = static_cast<float>(mPendingFlowField.mFlowFieldWidth) * mPendingFlowField.mSpacing;
	mPendingFlowField.mCellsTopLeftWorldPosition = targetPosition - glm::vec2{ fieldWidthWorldSpace } *.5f;
	mPendingFlowField.mFlowField.resize(static_cast<size_t>(Math::sqr(mPendingFlowField.mFlowFieldWidth)));

	// Set all to false
	mPendingIsBlocked.clear();
	mPendingIsBlocked.resize(mPendingFlowField.mFlowField.size());

	struct BoundingBox
	{
		glm::ivec2 mStart{};
		glm::ivec2 mEnd{};
	};

	static std::vector<BoundingBox> boxesToCheck{};
	boxesToCheck.clear();
	boxesToCheck.emplace_back(BoundingBox{ glm::ivec2{ 0 }, glm::ivec2{ mPendingFlowField.mFlowFieldWidth } });

	while (!boxesToCheck.empty())
	{
		BoundingBox box = boxesToCheck.back();
		boxesToCheck.pop_back();

		const TransformedAABB worldBoundingBox =
			{
				mPendingFlowField.GetCellBox(box.mStart.x, box.mStart.y).mMin,
				mPendingFlowField.GetCellBox(box.mEnd.x - 1, box.mEnd.y - 1).mMax
			};
		const char isTaken = bvh.Query(worldBoundingBox);

		if (!isTaken)
		{
			continue;
		}

		if (box.mStart + glm::ivec2{ 1 } == box.mEnd)
		{
			mPendingIsBlocked[box.mStart.x + box.mStart.y * mPendingFlowField.mFlowFieldWidth] = true;
			continue;
		}

		// Split and recurse
		const glm::ivec2 size = box.mEnd - box.mStart;

		BoundingBox children[2]{ box, box };

		const bool indexToChange = size.y > size.x;
		const int size1 = size[indexToChange] / 2;

		children[0].mEnd[indexToChange] = box.mStart[indexToChange] + size1;
		children[1].mStart[indexToChange] = children[0].mEnd[indexToChange];

		boxesToCheck.emplace_back(children[0]);
		boxesToCheck.emplace_back(children[1]);
	}

	mPendingThread = std::thread
	{
		[this, targetPosition, numOfSmoothingSteps = target.mNumberOfSmoothingSteps]
		{
			// A floodfill does not cover enclosed areas.
			// We initialize all cells with the direction
			// to the player
			for (int y = 0; y < mPendingFlowField.mFlowFieldWidth; y++)
			{
				for (int x = 0; x < mPendingFlowField.mFlowFieldWidth; x++)
				{
					const glm::vec2 cellCentre = mPendingFlowField.GetCellBox(x, y).GetCentre();
					glm::vec2 toTarget = targetPosition - cellCentre;

					if (toTarget == glm::vec2{})
					{
						toTarget = glm::vec2{ 1.0f, 0.0f };
					}

					mPendingFlowField.mFlowField[x + y * mPendingFlowField.mFlowFieldWidth] = glm::normalize(toTarget);
				}
			}

			const int centre = mPendingFlowField.mFlowFieldWidth / 2;
			const int startIndex = centre + centre * mPendingFlowField.mFlowFieldWidth;

			// Works functionally the same as a queue
			mPendingOpenIndices.clear();
			mPendingOpenIndices.emplace_back(startIndex);
			uint32 openCurrentIndex = 0;

			mPendingDistanceField.clear();
			mPendingDistanceField.resize(mPendingFlowField.mFlowField.size(), std::numeric_limits<float>::infinity());
			mPendingDistanceField[startIndex] = 0.0f;

			while (openCurrentIndex < mPendingOpenIndices.size())
			{
				const int current = mPendingOpenIndices[openCurrentIndex];
				openCurrentIndex++;

				// Pop_front in bulk
				if (openCurrentIndex >= 1024)
				{
					mPendingOpenIndices.erase(mPendingOpenIndices.begin(), mPendingOpenIndices.begin() + openCurrentIndex);
					openCurrentIndex = 0;
				}

				const float currentDist = mPendingDistanceField[current];

				const int x = current % mPendingFlowField.mFlowFieldWidth;
				const int y = current / mPendingFlowField.mFlowFieldWidth;

				const auto exploreNbr = [&](const int nbrX, const int nbrY, const float distIncease)
					{
						if (nbrY < 0
							|| nbrY >= mPendingFlowField.mFlowFieldWidth
							|| nbrX < 0
							|| nbrX >= mPendingFlowField.mFlowFieldWidth)
						{
							return;
						}

						const int nbrIndex = nbrX + nbrY * mPendingFlowField.mFlowFieldWidth;

						if (mPendingIsBlocked.data()[nbrIndex])
						{
							return;
						}

						const float nbrDist = currentDist + distIncease;

						if (nbrDist < mPendingDistanceField[nbrIndex])
						{
							mPendingDistanceField[nbrIndex] = nbrDist;
							mPendingOpenIndices.emplace_back(nbrIndex);
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

			for (int y = 0; y < mPendingFlowField.mFlowFieldWidth; y++)
			{
				for (int x = 0; x < mPendingFlowField.mFlowFieldWidth; x++)
				{
					float lowestDist = std::numeric_limits<float>::infinity();

					const auto checkNbr = [&](const int nbrX, const int nbrY)
						{
							if (nbrY < 0
								|| nbrY >= mPendingFlowField.mFlowFieldWidth
								|| nbrX < 0
								|| nbrX >= mPendingFlowField.mFlowFieldWidth)
							{
								return;
							}

							const int nbrIndex = nbrX + nbrY * mPendingFlowField.mFlowFieldWidth;
							const float dist = mPendingDistanceField[nbrIndex];

							if ((fabsf(lowestDist - dist) < 0.5f
								&& (nbrIndex & 1))
								|| dist < lowestDist)
							{
								lowestDist = dist;
								mPendingFlowField.mFlowField[x + y * mPendingFlowField.mFlowFieldWidth] = glm::normalize(glm::vec2{ static_cast<float>(nbrX - x), static_cast<float>(nbrY - y) });
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

			if (numOfSmoothingSteps == 0)
			{
				mIsPendingReady = true;
				return;
			}

			mSmoothedDirections = mPendingFlowField.mFlowField;

			for (int i = 0; i < numOfSmoothingSteps; i++)
			{
				for (int y = 0; y < mPendingFlowField.mFlowFieldWidth; y++)
				{
					for (int x = 0; x < mPendingFlowField.mFlowFieldWidth; x++)
					{
						glm::vec2 total = mPendingFlowField.mFlowField[x + y * mPendingFlowField.mFlowFieldWidth] * 2.0f;

						const auto addToTotal = [&](const int nbrX, const int nbrY)
							{
								if (nbrY < 0
									|| nbrY >= mPendingFlowField.mFlowFieldWidth
									|| nbrX < 0
									|| nbrX >= mPendingFlowField.mFlowFieldWidth)
								{
									total += glm::normalize(targetPosition - mPendingFlowField.GetCellBox(nbrX, nbrY).GetCentre());
									return;
								}

								const int nbrIndex = nbrX + nbrY * mPendingFlowField.mFlowFieldWidth;
								total += mPendingFlowField.mFlowField[nbrIndex];
							};

						addToTotal(x - 1, y - 1);
						addToTotal(x + 1, y + 1);
						addToTotal(x + 1, y - 1);
						addToTotal(x - 1, y + 1);

						addToTotal(x - 1, y);
						addToTotal(x, y - 1);
						addToTotal(x + 1, y);
						addToTotal(x, y + 1);

						glm::vec2 average = total * (1.0f / 10.0f);
						const float averageLength2 = glm::length2(average);

						if (averageLength2 == 0.0f)
						{
							continue;
						}

						mSmoothedDirections[x + y * mPendingFlowField.mFlowFieldWidth] = average / glm::sqrt(averageLength2);
					}
				}

				std::swap(mPendingFlowField.mFlowField, mSmoothedDirections);
			}
			mIsPendingReady = true;
		}
	};
}

void CE::SwarmingTargetSystem::Render(const World& world)
{
	if (!DebugRenderer::IsCategoryVisible(DebugCategory::AINavigation))
	{
		return;
	}

	for (auto [entity, target] : world.GetRegistry().View<SwarmingTargetComponent>().each())
	{
		for (int y = 0; y < target.mCurrent.mFlowFieldWidth; y++)
		{
			for (int x = 0; x < target.mCurrent.mFlowFieldWidth; x++)
			{
				const TransformedAABB cell = target.mCurrent.GetCellBox(x, y);

				const glm::vec2 pos = cell.GetCentre();
				const glm::vec2 dir = target.mCurrent.mFlowField[x + y * target.mCurrent.mFlowFieldWidth];

				DrawDebugLine(world, DebugCategory::AINavigation, pos, pos + dir * target.mCurrent.mSpacing * .75f, glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f });
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