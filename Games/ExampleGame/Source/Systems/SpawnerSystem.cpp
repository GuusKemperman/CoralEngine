#include "Precomp.h"
#include "Systems/SpawnerSystem.h"

#include "Assets/Prefabs/Prefab.h"
#include "Components/PlayerComponent.h"
#include "Components/PrefabOriginComponent.h"
#include "Components/SpawnerComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Utilities/DrawDebugHelpers.h"
#include "World/Physics.h"
#include "World/Registry.h"

void Game::SpawnerSystem::Update(CE::World& world, float dt)
{
	CE::Registry& reg = world.GetRegistry();

	const CE::TransformComponent* playerTransform = reg.TryGet<CE::TransformComponent>(reg.View<CE::PlayerComponent>().front());

	if (playerTransform == nullptr)
	{
		return;
	}

	for (auto [_, spawnerComponent] : reg.View<SpawnerComponent>().each())
	{
		SpawnerComponent::Wave* previousWave{};
		SpawnerComponent::Wave* currentWave{};

		{ // Get the current wave
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

		}

		std::unordered_map<CE::AssetHandle<CE::Prefab>, uint32> enemyCount{};
		const glm::vec2 spawnerPos = playerTransform->GetWorldPosition2D();
		std::vector<entt::entity> waveOutputs{};

		uint32 numOfEnemies{};

		{ // Count the number of enemies, and teleport far away enemies back
			const auto enemyView = reg.View<CE::TransformComponent, CE::CharacterComponent, CE::PrefabOriginComponent>(entt::exclude_t<CE::PlayerComponent>{});
			for (const entt::entity entity : enemyView)
			{
				const CE::PrefabOriginComponent& origin = enemyView.get<CE::PrefabOriginComponent>(entity);
				const CE::AssetHandle<CE::Prefab> prefab = origin.TryGetPrefab();

				if (prefab == nullptr)
				{
					continue;
				}

				enemyCount[prefab]++;
				numOfEnemies++;

				// Enemies that are too far away get teleported back
				const CE::TransformComponent& transform = enemyView.get<CE::TransformComponent>(entity);

				if (glm::distance2(transform.GetWorldPosition2D(), spawnerPos) >= CE::Math::sqr(spawnerComponent.mMaxEnemyDistance))
				{
					waveOutputs.emplace_back(entity);
				}
			}
		}

		// Spawn the initial amount of enemies
		if (previousWave != currentWave)
		{
			for (const SpawnerComponent::Wave::EnemyType& enemyType : currentWave->mEnemies)
			{
				if (enemyType.mPrefab == nullptr)
				{
					continue;
				}

				const uint32 amountToSpawn = enemyType.mAmountToSpawnAtStartOfWave.value_or(0u);
				enemyCount[enemyType.mPrefab] += amountToSpawn;

				for (uint32 i = 0; i < amountToSpawn; i++)
				{
					waveOutputs.emplace_back(reg.CreateFromPrefab(*enemyType.mPrefab));
				}
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

		// Determine which entity to spawn based on their chance
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
				break;
			}

			waveOutputs.emplace_back(reg.CreateFromPrefab(*enemyToSpawn->mPrefab));
			enemyCount[enemyToSpawn->mPrefab]++;
		}

		// Generate points
		float distFromCentre = spawnerComponent.mMinSpawnRange;

		do
		{
			const uint32 maxNumberOfPointsInLayer = static_cast<uint32>(PI / asin(spawnerComponent.mSpacing / distFromCentre)) + 1;

			const uint32 numberOfPointsInLayer = spawnerComponent.mShouldSpawnInGroups ? 
				maxNumberOfPointsInLayer :
				glm::min(static_cast<uint32>(waveOutputs.size()), maxNumberOfPointsInLayer);

			const float angleStepSize = TWOPI / static_cast<float>(numberOfPointsInLayer);
			float angle{};

			for (uint32 pointNum = 0; 
				pointNum < numberOfPointsInLayer 
				&& !waveOutputs.empty();
				pointNum++, angle += angleStepSize)
			{
				const glm::vec2 worldPos = spawnerPos + CE::Math::AngleToVec2(angle + world.GetCurrentTimeScaled()) * distFromCentre;

				// Shuffle the enemies
				const uint32 index = CE::Random::Range(0u, static_cast<uint32>(waveOutputs.size()));
				const entt::entity spawnedEntity = waveOutputs[index];
				waveOutputs[index] = waveOutputs.back();
				waveOutputs.pop_back();

				CE::TransformComponent* transform = world.GetRegistry().TryGet<CE::TransformComponent>(spawnedEntity);

				if (transform == nullptr)
				{
					LOG(LogGame, Warning, "Enemy did not have a transform and could not be moved to spawn position");
					continue;
				}

				transform->SetWorldPosition(worldPos);

				transform->SetWorldScale(CE::Random::Range(spawnerComponent.mMinRandomScale, spawnerComponent.mMaxRandomScale));
			}

			distFromCentre += spawnerComponent.mSpacing;
		} while (!waveOutputs.empty());
	}
}

CE::MetaType Game::SpawnerSystem::Reflect()
{
	return CE::MetaType{CE::MetaType::T<SpawnerSystem>{}, "SpawnerSystem", CE::MetaType::Base<System>{}};
}
