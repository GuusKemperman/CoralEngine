#include "Precomp.h"
#include "Systems/EnvironmentGeneratorSystem.h"

#include "Utilities/PerlinNoise.h"

#include "Components/EnvironmentGeneratorComponent.h"
#include "Components/PlayerComponent.h"
#include "Components/TransformComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Meta/MetaType.h"
#include "Rendering/DebugRenderer.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Geometry2d.h"
#include "Utilities/Reflect/ReflectComponentType.h"

namespace Game::Internal
{
	struct PartOfGeneratedEnvironmentTag
	{
		friend CE::ReflectAccess;
		static CE::MetaType Reflect()
		{
			CE::MetaType metaType = CE::MetaType{ CE::MetaType::T<PartOfGeneratedEnvironmentTag>{}, "PartOfGeneratedEnvironmentTag" };
			CE::ReflectComponentType<PartOfGeneratedEnvironmentTag>(metaType);
			return metaType;
		}
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

	// Generate around camera
	const CE::TransformComponent* generatorTransform = reg.TryGet<CE::TransformComponent>(reg.View<CE::PlayerComponent>().front());

	if (generatorTransform == nullptr)
	{
		return;
	}

	EnvironmentGeneratorComponent& generator = reg.Get<EnvironmentGeneratorComponent>(generatorEntity);

	if (!generator.mSeed.has_value())
	{
		generator.mSeed = CE::Random::Value<uint32>();
	}
	std::mt19937 layerSeedGenerator{ *generator.mSeed };

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

	const auto& getNumOfCellsEachAxis = [&](const EnvironmentGeneratorComponent::Layer& layer)
		{
			return static_cast<uint32>(ceilf((generator.mGenerateRadius + generator.mGenerateRadius) / layer.mCellSize)) + 1u;
		};

	const auto& getLayerTopLeft = [&](const EnvironmentGeneratorComponent::Layer& layer)
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

	// Only regenerate if we have moved a sufficiently large distance
	if (!world.HasBegunPlay()
		|| glm::distance2(generatorPosition, generator.mLastGeneratedAtPosition) < CE::Math::sqr(generator.mDistToMoveBeforeRegeneration))
	{
		return;
	}
	generator.mLastGeneratedAtPosition = generatorPosition;

	// Clear the terrain
	const auto createdEntities = reg.View<Internal::PartOfGeneratedEnvironmentTag>();
	reg.Destroy(createdEntities.begin(), createdEntities.end(), true);

	for (uint32 i = 0; i < static_cast<uint32>(generator.mLayers.size()); i++)
	{
		const EnvironmentGeneratorComponent::Layer& layer = generator.mLayers[i];
		const uint32 layerSeed = layerSeedGenerator();

		if (layer.mObjects.empty())
		{
			continue;
		}

		const uint32 numOfCellsEachAxis = getNumOfCellsEachAxis(layer);

		const glm::vec2 topLeft = getLayerTopLeft(layer);

		const CE::TransformedDisk generationCircle{ generatorPosition, generator.mGenerateRadius };

		for (uint32 cellX = 0; cellX < numOfCellsEachAxis; cellX++)
		{
			for (uint32 cellZ = 0; cellZ < numOfCellsEachAxis; cellZ++)
			{
				const glm::vec2 worldPosition = topLeft + layer.mCellSize *
					glm::vec2{ static_cast<float>(cellX), static_cast<float>(cellZ) };

				const CE::TransformedAABB cellAABB{ worldPosition, worldPosition + glm::vec2{ layer.mCellSize } };

				if (!CE::AreOverlapping(cellAABB, generationCircle))
				{
					continue;
				}

				const glm::vec<2, int64> seedVec = static_cast<glm::vec<2, int64>>(worldPosition / layer.mCellSize * 1234.987654321f);
				const uint32 combinedHash = static_cast<uint32>(seedVec.y ^ (seedVec.x + 0x9e3779b9 + (seedVec.y << 6) + (seedVec.y >> 2)));

				std::mt19937 cellGenerator{ CE::Internal::CombineHashes(combinedHash, layerSeed) };

				const auto randomUint = [&cellGenerator](uint32 min, uint32 max)
					{
						std::uniform_int_distribution distribution{ min, max == min ? max : max - 1 };
						return distribution(cellGenerator);
					};

				const auto randomFloat = [&cellGenerator](float min, float max)
					{
						std::uniform_real_distribution distribution{ min, max };
						return distribution(cellGenerator);
					};

				const glm::vec2 spawnPosition = cellAABB.GetCentre() +
					glm::vec2{
						randomFloat(-layer.mMaxRandomOffset, layer.mMaxRandomOffset),
						randomFloat(-layer.mMaxRandomOffset, layer.mMaxRandomOffset)
				};

				const std::optional<float> noise = getNoise(getNoise, spawnPosition, i);

				if (!noise.has_value())
				{
					continue;
				}

				CE::WeightedRandomDistribution<std::reference_wrapper<const EnvironmentGeneratorComponent::Layer::Object>> distribution{};

				for (const EnvironmentGeneratorComponent::Layer::Object& object : layer.mObjects)
				{
					distribution.mWeights.emplace_back(std::cref(object), object.mBaseFrequency * object.mSpawnFrequenciesAtNoiseValue.GetValueAt(*noise));
				}

				const auto* objectToSpawn = distribution.GetNext(randomFloat(0.0f, 1.0f));

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

				const entt::entity entity = reg.CreateFromPrefab(*prefabToSpawn);

				if (entity == entt::null)
				{
					continue;
				}

				reg.AddComponent<Internal::PartOfGeneratedEnvironmentTag>(entity);

				CE::TransformComponent* objectTransform = reg.TryGet<CE::TransformComponent>(entity);

				if (objectTransform == nullptr)
				{
					continue;
				}

				const glm::vec3 scale = glm::vec3{ objectToSpawn->get().mScaleAtNoiseValue.GetValueAt(*noise) };
				const uint32 orientation = randomUint(1u, layer.mNumberOfRandomRotations);
				const float angle = static_cast<float>(orientation) * (TWOPI / static_cast<float>(layer.mNumberOfRandomRotations));

				objectTransform->SetWorldPosition(spawnPosition);
				objectTransform->SetLocalOrientation(CE::sUp * angle);
				objectTransform->SetWorldScale(scale);
			}
		}
	}
}

CE::MetaType Game::EnvironmentGeneratorSystem::Reflect()
{
	return CE::MetaType{ CE::MetaType::T<EnvironmentGeneratorSystem>{}, "EnvironmentGeneratorSystem", CE::MetaType::Base<System>{} };
}