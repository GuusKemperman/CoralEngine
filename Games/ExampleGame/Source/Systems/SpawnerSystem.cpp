#include "Precomp.h"
#include "Systems/SpawnerSystem.h"

#include "Components/PlayerComponent.h"
#include "Components/SpawnerComponent.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"

void Game::SpawnerSystem::Update(CE::World& world, float)
{
	auto& reg = world.GetRegistry();
	auto spawnerView = reg.View<SpawnerComponent, CE::TransformComponent>();

	const auto playerCheck = reg.View<PlayerComponent>();

	if (playerCheck.empty()) { return; }

	const auto playerView = reg.View<PlayerComponent, CE::TransformComponent>();

	auto [playerComponent, playerTransform] = playerView.get(playerView.front());

	for (auto [spawnerID, spawnerComponent, spawnerTransform] : spawnerView.each())
	{
		const float distance = glm::distance(playerTransform.GetWorldPosition(),
		                                     spawnerTransform.GetWorldPosition());

		if (spawnerComponent.mMax > distance && spawnerComponent.mMin < distance)
		{
			spawnerComponent.mActive = true;
		}
		else
		{
			spawnerComponent.mActive = false;
		}
	}
}

CE::MetaType Game::SpawnerSystem::Reflect()
{
	return CE::MetaType{CE::MetaType::T<SpawnerSystem>{}, "SpawnerSystem", CE::MetaType::Base<System>{}};
}
