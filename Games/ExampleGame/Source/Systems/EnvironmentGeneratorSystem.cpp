#include "Precomp.h"
#include "Systems/EnvironmentGeneratorSystem.h"

#include "Utilities/PerlinNoise.h"

#include "Components/EnvironmentGeneratorComponent.h"
#include "Components/MeshColorComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Input.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Rendering/DebugRenderer.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Geometry2d.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Physics.h"

namespace Game::Internal
{
	struct PartOfGeneratedEnvironmentComponent
	{
		uint32 mLayerIndex{};
		glm::vec2 mCellTopLeft{};

		friend CE::ReflectAccess;
		static CE::MetaType Reflect()
		{
			CE::MetaType metaType = CE::MetaType{ CE::MetaType::T<PartOfGeneratedEnvironmentComponent>{}, "PartOfGeneratedEnvironmentComponent" };
			CE::ReflectComponentType<PartOfGeneratedEnvironmentComponent>(metaType);
			return metaType;
		}
		REFLECT_AT_START_UP(PartOfGeneratedEnvironmentComponent);
	};
}

void Game::EnvironmentGeneratorSystem::Update(CE::World& world, float)
{
	CE::Registry& reg = world.GetRegistry();

	const entt::entity generatorEntity = reg.View<EnvironmentGeneratorComponent>().front();

	if (generatorEntity == entt::null)
	{
		return;
	}

	const CE::TransformComponent* generatorTransform = reg.TryGet<CE::TransformComponent>(reg.View<CE::PlayerComponent>().front());

	if (generatorTransform == nullptr)
	{
		return;
	}

	EnvironmentGeneratorComponent& generator = reg.Get<EnvironmentGeneratorComponent>(generatorEntity);

	std::mt19937 layerSeedGenerator{ generator.mSeed };

	std::vector<siv::PerlinNoise> perlinGenerators{};
	perlinGenerators.reserve(generator.mLayers.size());
	for (size_t i = 0; i < generator.mLayers.size(); i++)
	{
		perlinGenerators.emplace_back(layerSeedGenerator());
	}

	const auto getNoise = [&](const auto& self, glm::vec2 position, uint32 indexOfLayer, uint32 depth = 0) -> std::optional<float>
		{
			if (indexOfLayer >= perlinGenerators.size())
			{
				LOG(LogGame, Error, "Cannot sample noise at index {}, there are only {} layers", indexOfLayer, perlinGenerators.size());
				return std::nullopt;
			}

			if (depth > perlinGenerators.size())
			{
				LOG(LogGame, Error, "Feedback loop detected, some noise influences map to eachother.", indexOfLayer, perlinGenerators.size());
				return std::nullopt;
			}

			const EnvironmentGeneratorComponent::Layer& layer = generator.mLayers[indexOfLayer];

			const float noiseSample = perlinGenerators[indexOfLayer].octave2D_01(position.x * layer.mNoiseScale, position.y * layer.mNoiseScale, layer.mNoiseNumOfOctaves, layer.mNoisePersistence);
			float noise = noiseSample * layer.mWeight;
			float max = layer.mWeight;

			for (const EnvironmentGeneratorComponent::Layer::NoiseInfluence& influence : layer.mInfluences)
			{
				std::optional<float> otherSample = self(self, position, influence.mIndexOfOtherLayer, depth + 1);

				if (otherSample == std::nullopt)
				{
					return std::nullopt;
				}

				noise += *otherSample * influence.mWeight;
				max += influence.mWeight;
			}

			if (max == 0.0f)
			{
				return 0.0f;
			}
			// Normalized
			return noise / max;
		};
	const glm::vec2 generatorPosition = generatorTransform->GetWorldPosition2D();

	static constexpr CE::DebugCategory::Enum debugCategory = CE::DebugCategory::Editor;

	const auto getNumOfCellsEachAxis = [&](const EnvironmentGeneratorComponent::Layer& layer)
		{
			return static_cast<uint32>(ceilf((generator.mGenerateRadius + generator.mGenerateRadius) / layer.mCellSize)) + 1u;
		};

	const auto getLayerTopLeft = [&](const EnvironmentGeneratorComponent::Layer& layer)
		{
			const glm::vec2 generatorCellPos = generatorPosition - glm::vec2{ fmodf(generatorPosition.x, layer.mCellSize), fmodf(generatorPosition.y, layer.mCellSize) };
			return generatorCellPos - glm::vec2{ static_cast<float>(getNumOfCellsEachAxis(layer)) * .5f * layer.mCellSize };
		};

	if (CE::DebugRenderer::IsCategoryVisible(debugCategory))
	{
		for (uint32 i = 0; i < static_cast<uint32>(generator.mLayers.size()); i++)
		{
			const EnvironmentGeneratorComponent::Layer& layer = generator.mLayers[i];

			if (!layer.mIsDebugDrawingEnabled)
			{
				continue;
			}

			const glm::vec4 colour =
			{
				(i & 1) ? 0.0f : 1.0f,
				((i >> 1) & 1) ? 0.0f : 1.0f,
				((i >> 2) & 1) ? 0.0f : 1.0f,
				1.0f
			};

			const uint32 numOfCellsEachAxis = getNumOfCellsEachAxis(layer);
			const glm::vec2 topLeft = getLayerTopLeft(layer);

			for (uint32 cellX = 0; cellX < numOfCellsEachAxis; cellX++)
			{
				for (uint32 cellZ = 0; cellZ < numOfCellsEachAxis; cellZ++)
				{
					const glm::vec2 x0z0Pos2D = topLeft + layer.mCellSize *
						glm::vec2{ static_cast<float>(cellX), static_cast<float>(cellZ) };

					const glm::vec2 x1z0Pos2D = { x0z0Pos2D.x + layer.mCellSize, x0z0Pos2D.y };
					const glm::vec2 x0z1Pos2D = { x0z0Pos2D.x, x0z0Pos2D.y + layer.mCellSize };

					const std::optional<float> x0z0Noise = getNoise(getNoise, x0z0Pos2D, i);
					const std::optional<float> x1z0Noise = getNoise(getNoise, x1z0Pos2D, i);
					const std::optional<float> x0z1Noise = getNoise(getNoise, x0z1Pos2D, i);

					if (!x0z0Noise.has_value()
						|| !x1z0Noise.has_value()
						|| !x0z1Noise.has_value())
					{
						continue;
					}

					const float layerHeight = 3.0f + static_cast<float>((static_cast<uint32>(generator.mLayers.size() - 1) - i)) * generator.mDebugDrawDistanceBetweenLayers;
					const glm::vec3 x0z0 = CE::To3DRightForward(x0z0Pos2D, *x0z0Noise * generator.mDebugDrawNoiseHeight + layerHeight);
					const glm::vec3 x1z0 = CE::To3DRightForward(x1z0Pos2D, *x1z0Noise * generator.mDebugDrawNoiseHeight + layerHeight);
					const glm::vec3 x0z1 = CE::To3DRightForward(x0z1Pos2D, *x0z1Noise * generator.mDebugDrawNoiseHeight + layerHeight);

					CE::DrawDebugLine(world, debugCategory, x0z0, x1z0, colour);
					CE::DrawDebugLine(world, debugCategory, x0z0, x0z1, colour);
				}
			}
		}
	}

	if (generator.mWasClearingRequested)
	{
		auto createdEntities = reg.View<Internal::PartOfGeneratedEnvironmentComponent>();
		reg.Destroy(createdEntities.begin(), createdEntities.end(), true);
		reg.RemovedDestroyed();
		generator.mWasClearingRequested = false;
	}

	// Only regenerate if we have moved a sufficiently large distance
	if (glm::distance2(generatorPosition, generator.mLastGeneratedAtPosition) < CE::Math::sqr(generator.mDistToMoveBeforeRegeneration))
	{
		return;
	}
	generator.mLastGeneratedAtPosition = generatorPosition;

	// For each layer, an std::vector<char> representing a 2D grid.
	// Only the 0 chars need to be filled in.
	static std::vector<std::vector<char>> isEnvironmentFilledIn{};
	isEnvironmentFilledIn.resize(generator.mLayers.size());

	for (uint32 i = 0; i < static_cast<uint32>(generator.mLayers.size()); i++)
	{
		isEnvironmentFilledIn[i].clear();

		const uint32 numOfCellsEachAxis = getNumOfCellsEachAxis(generator.mLayers[i]);
		isEnvironmentFilledIn[i].resize(CE::Math::sqr(numOfCellsEachAxis));
	}

	const CE::TransformedDisk generationCircle{ generatorPosition, generator.mGenerateRadius };
	const CE::TransformedDisk destroyCircle{ generatorPosition, generator.mDestroyRadius };

	for (const auto [entity, generatedComponent] : reg.View<Internal::PartOfGeneratedEnvironmentComponent>().each())
	{
		if (generatedComponent.mLayerIndex >= isEnvironmentFilledIn.size())
		{
			reg.Destroy(entity, true);
			continue;
		}

		const EnvironmentGeneratorComponent::Layer& layer = generator.mLayers[generatedComponent.mLayerIndex];
		const CE::TransformedAABB cellAABB{ generatedComponent.mCellTopLeft, generatedComponent.mCellTopLeft + glm::vec2{ layer.mCellSize } };

		if (!CE::AreOverlapping(cellAABB, destroyCircle))
		{
			reg.Destroy(entity, true);
			continue;
		}

		std::vector<char>& grid = isEnvironmentFilledIn[generatedComponent.mLayerIndex];

		const glm::vec2 gridTopLeft = getLayerTopLeft(layer);
		const int gridSize = getNumOfCellsEachAxis(layer);

		const glm::vec2 floatIndex = (generatedComponent.mCellTopLeft + glm::vec2{ layer.mCellSize * .5f } - gridTopLeft) / layer.mCellSize;

		if (floatIndex.x < 0.0f
			|| floatIndex.y < 0.0f)
		{
			continue;
		}

		const int indexX = static_cast<int>(floatIndex.x);
		const int indexY = static_cast<int>(floatIndex.y);

		if (indexX >= gridSize
			|| indexY >= gridSize)
		{
			continue;
		}

		char& isFilledIn = grid[indexX + indexY * gridSize];
		++isFilledIn;
		if (isFilledIn > 1)
		{
			LOG(LogGame, Error, "Multiple cells mapped to the same cell, some terrain may not be properly cleared or some cells may be spawned twice");
		}
	}

	// Clear the terrain
	if (!world.HasBegunPlay()
		&& !generator.mShouldGenerateInEditor)
	{
		return;
	}

	const std::array<std::reference_wrapper<const CE::BVH>, 2> bvhs =
	{
		world.GetPhysics().GetBVHs()[static_cast<int>(CE::CollisionLayer::StaticObstacles)],
		world.GetPhysics().GetBVHs()[static_cast<int>(CE::CollisionLayer::Terrain)]
	};

	bool areBVHsDirty = true;

	const auto& isBlocked = [&reg, &bvhs, &areBVHsDirty](const auto& self, const CE::TransformComponent& current) -> bool
		{
			if (const auto* collider = reg.TryGet<CE::AABBColliderComponent>(current.GetOwner());
				collider != nullptr)
			{
				const auto transformed = collider->CreateTransformedCollider(current);
				areBVHsDirty = true; // A collider was present on a newly created entity

				for (const CE::BVH& bvh : bvhs)
				{
					if (bvh.Query(transformed))
					{
						return true;
					}
				}
			}

			if (const auto* collider = reg.TryGet<CE::DiskColliderComponent>(current.GetOwner());
				collider != nullptr)
			{
				const auto transformed = collider->CreateTransformedCollider(current);
				areBVHsDirty = true; // A collider was present on a newly created entity

				for (const CE::BVH& bvh : bvhs)
				{
					if (bvh.Query(transformed))
					{
						return true;
					}
				}
			}

			if (const auto* collider = reg.TryGet<CE::PolygonColliderComponent>(current.GetOwner());
				collider != nullptr)
			{
				const auto transformed = collider->CreateTransformedCollider(current);
				areBVHsDirty = true; // A collider was present on a newly created entity

				for (const CE::BVH& bvh : bvhs)
				{
					if (bvh.Query(transformed))
					{
						return true;
					}
				}
			}

			for (const CE::TransformComponent& child : current.GetChildren())
			{
				if (self(self, child))
				{
					return true;
				}
			}

			return false;
		};
	CE::WeightedRandomDistribution<std::reference_wrapper<const EnvironmentGeneratorComponent::Layer::Object>> distribution{};

	for (uint32 i = 0; i < static_cast<uint32>(generator.mLayers.size()); i++)
	{
		if (areBVHsDirty)
		{
			reg.RemovedDestroyed();
			world.GetPhysics().RebuildBVHs();
			areBVHsDirty = false;
		}

		const EnvironmentGeneratorComponent::Layer& layer = generator.mLayers[i];
		const uint32 layerSeed = layerSeedGenerator();

		if (layer.mObjects.empty())
		{
			continue;
		}

		distribution.mWeights.clear();
		for (const EnvironmentGeneratorComponent::Layer::Object& object : layer.mObjects)
		{
			distribution.mWeights.emplace_back(std::cref(object), object.mFrequency);
		}

		const uint32 numOfCellsEachAxis = getNumOfCellsEachAxis(layer);

		const glm::vec2 topLeft = getLayerTopLeft(layer);

		for (uint32 cellX = 0; cellX < numOfCellsEachAxis; cellX++)
		{
			for (uint32 cellZ = 0; cellZ < numOfCellsEachAxis; cellZ++)
			{
				if (isEnvironmentFilledIn[i][cellX + cellZ * numOfCellsEachAxis])
				{
					continue;
				}

				const glm::vec2 worldPosition = topLeft + layer.mCellSize *
					glm::vec2{ static_cast<float>(cellX), static_cast<float>(cellZ) };


				const CE::TransformedAABB cellAABB{ worldPosition, worldPosition + glm::vec2{ layer.mCellSize } };

				if (!CE::AreOverlapping(cellAABB, generationCircle))
				{
					continue;
				}

				CE::DefaultRandomEngine cellGenerator{ CE::Internal::CombineHashes(CE::Random::CreateSeed(worldPosition), layerSeed) };

				const auto randomFloat = [&cellGenerator](float min, float max)
					{
						std::uniform_real_distribution distribution{ min, std::max(min, max) };
						return distribution(cellGenerator);
					};

				if (randomFloat(0.0f, 1.0f) > layer.mObjectSpawnChance)
				{
					continue;
				}

				const auto randomUint = [&cellGenerator](uint32 min, uint32 max)
					{
						std::uniform_int_distribution distribution{ min, std::max(min, max == min ? max : max - 1) };
						return distribution(cellGenerator);
					};

				const glm::vec2 spawnPosition2D = cellAABB.GetCentre() +
					glm::vec2{
						randomFloat(-layer.mMaxRandomOffset, layer.mMaxRandomOffset),
						randomFloat(-layer.mMaxRandomOffset, layer.mMaxRandomOffset)
				};

				const std::optional<float> noise = getNoise(getNoise, spawnPosition2D, i);

				if (!noise.has_value()
					|| *noise < layer.mMinNoiseValueToSpawn
					|| *noise > layer.mMaxNoiseValueToSpawn)
				{
					continue;
				}

				const auto* objectToSpawn = distribution.GetNext( randomFloat(0.0f, 1.0f));

				if (objectToSpawn == nullptr)
				{
					LOG(LogGame, Error, "Object to spawn was unexpectedly nullptr");
					continue;
				}

				const CE::AssetHandle<CE::Prefab> prefabToSpawn = objectToSpawn->get().mPrefab;

				if (prefabToSpawn == nullptr)
				{
					continue;
				}

				const float angle = static_cast<float>(randomUint(1u, layer.mNumberOfRandomRotations)) * (TWOPI / static_cast<float>(layer.mNumberOfRandomRotations));
				glm::quat quatOrientation = glm::quat{ CE::sUp * angle };
				quatOrientation *= glm::quat{ CE::sRight * glm::radians(randomFloat(layer.mMinRandomOrientation.x, layer.mMaxRandomOrientation.x)) };
				quatOrientation *= glm::quat{ CE::sForward * glm::radians(randomFloat(layer.mMinRandomOrientation.y, layer.mMaxRandomOrientation.y)) };

				const glm::vec3 scale = glm::vec3{ layer.mScaleAtNoiseValue.GetValueAt( CE::Math::lerpInv(layer.mMinNoiseValueToSpawn, layer.mMaxNoiseValueToSpawn, *noise)) };
				const glm::vec3 spawnPosition3D = CE::To3DRightForward(spawnPosition2D, randomFloat(layer.mMinRandomHeightOffset, layer.mMaxRandomHeightOffset));

				entt::entity entity = reg.CreateFromPrefab(*prefabToSpawn, entt::null, &spawnPosition3D, &quatOrientation, &scale);

				if (entity == entt::null)
				{
					continue;
				}
				
				const CE::TransformComponent* transform = reg.TryGet<CE::TransformComponent>(entity);

				if (transform != nullptr
					&& isBlocked(isBlocked, *transform))
				{
					reg.Destroy(entity, true);

					// We still have to mark this spot as 'taken'
					// Otherwise we would try to spawn this prefab
					// again the very next time we move.
					entity = reg.Create();
					reg.AddComponent<Internal::PartOfGeneratedEnvironmentComponent>(entity, i, cellAABB.mMin);
					continue;
				}

				reg.AddComponent<Internal::PartOfGeneratedEnvironmentComponent>(entity, i, cellAABB.mMin);

				if (transform == nullptr)
				{
					continue;
				}

				if (layer.mColors.empty())
				{
					continue;
				}


				const CE::LinearColor color = layer.mColors[randomUint(0u, static_cast<uint32>(layer.mColors.size()))];

				if (color == glm::vec4{ 1.0f })
				{
					continue;
				}

				const auto addColor = [&reg, &color](const auto& self, const CE::TransformComponent& current) -> void
					{
						if (!reg.HasComponent<CE::MeshColorComponent>(current.GetOwner()))
						{
							reg.AddComponent<CE::MeshColorComponent>(current.GetOwner(), color);
						}

						for (const CE::TransformComponent& child : current.GetChildren())
						{
							self(self, child);
						}
					};
				addColor(addColor, *transform);
			}
		}
	}
}

CE::MetaType Game::EnvironmentGeneratorSystem::Reflect()
{
	return CE::MetaType{ CE::MetaType::T<EnvironmentGeneratorSystem>{}, "EnvironmentGeneratorSystem", CE::MetaType::Base<System>{} };
}