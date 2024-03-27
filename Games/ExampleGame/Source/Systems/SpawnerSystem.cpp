#include "Precomp.h"
#include "Systems/SpawnerSystem.h"

#include "Components/SpawnerComponent.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"

void Game::SpawnerSystem::Update(Engine::World& world, float dt)
{
	Engine::Registry& reg = world.GetRegistry();
	auto spawnerView = reg.View<SpawnerComponent, Engine::TransformComponent>();
	for (auto [spawnerID, spawnerComponent, spawnerTransform] : spawnerView.each())
	{
		if (spawnerComponent.mPrefab == nullptr)
		{
			continue;
		}

		spawnerComponent.mCurrentTimer += dt;
		if (spawnerComponent.mCurrentTimer < spawnerComponent.mSpawningTimer)
		{
			continue;
		}
		
		const entt::entity spawnedPrefab = reg.CreateFromPrefab(*spawnerComponent.mPrefab);
		spawnerComponent.mCurrentTimer = 0;

		const TransformComponent* const spawnedTransform = reg.TryGet<Engine::TransformComponent>(spawnedPrefab);

		if (spawnedTransform != nullptr)
		{
			spawnedTransform->SetWorldPosition(spawnerTransform.GetWorldPosition());
		}
	}
}

Engine::MetaType Game::SpawnerSystem::Reflect()
{
	return Engine::MetaType{Engine::MetaType::T<SpawnerSystem>{}, "SpawnerSystem", Engine::MetaType::Base<System>{}};
}
