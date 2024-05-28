#include "Precomp.h"
#include "Systems/SpawnerSystem.h"

#include "Assets/Prefabs/Prefab.h"
#include "Components/PlayerComponent.h"
#include "Components/PrefabOriginComponent.h"
#include "Components/SpawnerComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Utilities/DrawDebugHelpers.h"
#include "World/Registry.h"

void Game::SpawnerSystem::Update(CE::World& world, float dt)
{
	CE::Registry& reg = world.GetRegistry();


	std::unordered_map<CE::AssetHandle<CE::Prefab>, uint32> enemyCount{};

	uint32 numOfEnemies{};
	const auto enemyView = reg.View<CE::CharacterComponent, CE::PrefabOriginComponent>(entt::exclude_t<CE::PlayerComponent>{});
	for (const entt::entity entity : enemyView)
	{
		const CE::PrefabOriginComponent& origin = enemyView.get<CE::PrefabOriginComponent>(entity);
		const CE::AssetHandle<CE::Prefab> prefab = origin.TryGetPrefab();

		if (prefab != nullptr)
		{
			enemyCount[prefab]++;
			numOfEnemies++;
		}
	}

	for (auto [entity, spawnerComponent, spawnerTransform] : reg.View<SpawnerComponent, CE::TransformComponent>().each())
	{
		SpawnerComponent::Wave* previousWave{};
		SpawnerComponent::Wave* currentWave{};

		const float currentTime = world.GetCurrentTimeScaled();
		const float previousTime = currentTime - dt;

		float waveTimeAccumulated{};

		for (SpawnerComponent::Wave& wave : spawnerComponent.mWaves)
		{
			const float nextWaveTimeAccumulated = waveTimeAccumulated + wave.mDuration;

			if (currentTime >= waveTimeAccumulated
				&& currentTime <= nextWaveTimeAccumulated)
			{
				currentWave = &wave;
			}

			if (previousTime >= waveTimeAccumulated
				&& previousTime <= nextWaveTimeAccumulated)
			{
				previousWave = &wave;
			}

			waveTimeAccumulated = nextWaveTimeAccumulated;
		}

		if (currentWave == nullptr)
		{
			LOG(LogGame, Message, "There is no active wave for the current time");
			continue;
		}

		struct WaveOutput
		{
			CE::AssetHandle<CE::Prefab> mPrefab{};
			uint32 mAmount{};
		};

		std::vector<CE::AssetHandle<CE::Prefab>> waveOutputs{};

		if (previousWave != currentWave)
		{
			for (const SpawnerComponent::Wave::EnemyType& enemyType : currentWave->mEnemies)
			{
				if (enemyType.mPrefab == nullptr)
				{
					continue;
				}

				const uint32 amountToSpawn = enemyType.mAmountToSpawnAtStartOfWave.value_or(0u);
				waveOutputs.insert(waveOutputs.end(), amountToSpawn, enemyType.mPrefab);
			}
		}

		const float spawnRate = numOfEnemies < currentWave->mDesiredMinimumNumberOfEnemies
			? currentWave->mAmountToSpawnPerSecondWhenBelowMinimum
			: currentWave->mAmountToSpawnPerSecond;

		const uint32 numOfObjects = static_cast<uint32>(spawnRate * dt);

		CE::WeightedRandomDistribution<SpawnerComponent::Wave::EnemyType> distribution{};

		for (const SpawnerComponent::Wave::EnemyType& enemyType : currentWave->mEnemies)
		{
			if (enemyType.mPrefab != nullptr)
			{
				distribution.mWeights.emplace_back(enemyType, enemyType.mSpawnChance);
			}
		}

		for (uint32 i = 0; i < numOfObjects; i++)
		{
			distribution.mWeights.erase(std::remove_if(distribution.mWeights.begin(), distribution.mWeights.end(),
				[&enemyCount](const std::pair<SpawnerComponent::Wave::EnemyType, float>& item)
				{
					if (!item.first.mMaxAmountAlive.has_value())
					{
						return false;
					}

					const uint32 numAlive = enemyCount[item.first.mPrefab];
					return numAlive >= *item.first.mMaxAmountAlive;
				}), distribution.mWeights.end());

			SpawnerComponent::Wave::EnemyType* enemyToSpawn = distribution.GetNext();

			if (enemyToSpawn == nullptr)
			{
				LOG(LogGames, Error, "No enemy to spawn!");
				return;
			}

			waveOutputs.emplace_back(enemyToSpawn->mPrefab);
			enemyCount[enemyToSpawn->mPrefab]++;
		}

		const glm::vec2 spawnerPos = spawnerTransform.GetWorldPosition2D();

		uint32 numOfObjectsSpawned{};

		// Generate points
		float distFromCentre = spawnerComponent.mMinSpawnRange;

		do
		{
			const uint32 numberOfPointsInLayer = static_cast<uint32>(PI / asin(spawnerComponent.mSpacing / distFromCentre)) + 1;
			const float angleStepSize = TWOPI / static_cast<float>(numberOfPointsInLayer);
			float angle{};

			for (uint32 pointNum = 0; 
				pointNum < numberOfPointsInLayer 
				&& numOfObjectsSpawned < numOfObjects;
				pointNum++, numOfObjectsSpawned++, angle += angleStepSize)
			{
				const CE::AssetHandle<CE::Prefab>& enemyToSpawn = waveOutputs[numOfObjectsSpawned];

				if (enemyToSpawn == nullptr)
				{
					continue;
				}

				const glm::vec2 worldPos = spawnerPos + CE::Math::AngleToVec2(angle + world.GetCurrentTimeScaled()) * distFromCentre;

				const entt::entity spawnedEntity = world.GetRegistry().CreateFromPrefab(*enemyToSpawn);
				CE::TransformComponent* transform = world.GetRegistry().TryGet<CE::TransformComponent>(spawnedEntity);

				CE::DrawDebugCircle(world, CE::DebugCategory::Gameplay, CE::To3DRightForward(worldPos), 1.0f, glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f });

				if (transform != nullptr)
				{
					transform->SetWorldPosition(worldPos);
				}
			}

			distFromCentre += spawnerComponent.mSpacing;
		} while (numOfObjectsSpawned < numOfObjects);
	}
}

CE::MetaType Game::SpawnerSystem::Reflect()
{
	return CE::MetaType{CE::MetaType::T<SpawnerSystem>{}, "SpawnerSystem", CE::MetaType::Base<System>{}};
}
