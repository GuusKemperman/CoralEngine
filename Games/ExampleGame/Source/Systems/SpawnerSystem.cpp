#include "Precomp.h"
#include "Systems/SpawnerSystem.h"

#include "Components/SpawnerComponent.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"

void Game::SpawnerSystem::Update(CE::World& world, float dt)
{
	CE::Registry& reg = world.GetRegistry();
	auto spawnerView = reg.View<SpawnerComponent, CE::TransformComponent>();
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

		CE::TransformComponent* const spawnedTransform = reg.TryGet<CE::TransformComponent>(spawnedPrefab);

		if (spawnedTransform != nullptr)
		{
			spawnedTransform->SetWorldPosition(spawnerTransform.GetWorldPosition());
		}
	}
}

CE::MetaType Game::SpawnerSystem::Reflect()
{
	return CE::MetaType{CE::MetaType::T<SpawnerSystem>{}, "SpawnerSystem", CE::MetaType::Base<System>{}};
}
