#include "Precomp.h"
#include "Systems/SpawnerSystem.h"

#include "Components/SpawnerComponent.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"

void Engine::SpawnerSystem::Update(World& world, float dt)
{
	Registry& reg = world.GetRegistry();
	auto spawnerView = reg.View<SpawnerComponent, TransformComponent>();
	for (auto [spawnerID, spawnerComponent, spawnerTransform] : spawnerView.each())
	{
		if (spawnerComponent.mEnemyPrefab == nullptr)
		{
			continue;
		}

		spawnerComponent.mCurrentTimer += dt;
		if (spawnerComponent.mCurrentTimer >= spawnerComponent.mSpawningTimer)
		{
			entt::entity spawnedPrefab = reg.CreateFromPrefab(*spawnerComponent.mEnemyPrefab);

			auto* spawnedTransform = reg.TryGet<TransformComponent>(spawnedPrefab);

			spawnedTransform->SetWorldPosition(spawnedTransform->GetWorldPosition());
			spawnerComponent.mCurrentTimer = 0;
		}
	}
}

Engine::MetaType Engine::SpawnerSystem::Reflect()
{
	return MetaType{MetaType::T<SpawnerSystem>{}, "SpawnerSystem", MetaType::Base<System>{}};
}
