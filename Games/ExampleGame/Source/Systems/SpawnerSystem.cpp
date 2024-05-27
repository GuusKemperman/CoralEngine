#include "Precomp.h"
#include "Systems/SpawnerSystem.h"

#include "Components/SpawnerComponent.h"
#include "Components/TransformComponent.h"
#include "Utilities/DrawDebugHelpers.h"
#include "World/Registry.h"

void Game::SpawnerSystem::Update(CE::World& world, float dt)
{
	CE::Registry& reg = world.GetRegistry();

	for (auto [entity, spawnerComponent, spawnerTransform] : reg.View<SpawnerComponent, CE::TransformComponent>().each())
	{
		const glm::vec2 spawnerPos = spawnerTransform.GetWorldPosition2D();
		const uint32 numOfObjects = static_cast<uint32>(dt * spawnerComponent.mAmountToSpawnPerSecond);
		uint32 numOfObjectsSpawned{};

		// Generate points
		float distFromCentre = spawnerComponent.mMinSpawnRange;
		do
		{
			const uint32 numberOfPointsInLayer = static_cast<uint32>(PI / asin(spawnerComponent.mSpacing / distFromCentre)) + 1;
			const float angleStepSize = TWOPI / static_cast<float>(numberOfPointsInLayer);
			float angle{};

			for (uint32 pointNum = 0; pointNum < numberOfPointsInLayer && numOfObjectsSpawned < numOfObjects; pointNum++, numOfObjectsSpawned++, angle += angleStepSize)
			{
				const glm::vec2 worldPos = spawnerPos + CE::Math::AngleToVec2(angle + world.GetCurrentTimeScaled()) * distFromCentre;

				const entt::entity spawnedEntity = world.GetRegistry().CreateFromPrefab(*spawnerComponent.mPrefabToSpawn);
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
