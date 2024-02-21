#include "Precomp.h"
#include "Systems/Particles/ParticleSpawnPrefabOnDeathSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleSpawnPrefabOnDeathComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::ParticleSpawnPrefabOnDeathSystem::Update(World& world, [[maybe_unused]] float dt)
{
	Registry& reg = world.GetRegistry();

	const auto view = reg.View<const ParticleEmitterComponent, const ParticleSpawnPrefabOnDeathComponent>();

	for (auto [entity, emitter, spawner] : view.each())
	{
		if (spawner.mPrefabToSpawn == nullptr)
		{
			continue;
		}

		const Span<const size_t> particlesThatJustDied = emitter.GetParticlesThatDiedDuringLastStep();
		const Span<const glm::vec3> positions = emitter.GetParticlePositions();
		const Span<const glm::vec3> scales = emitter.GetParticleSizes();
		const Span<const glm::quat> orientations = emitter.GetParticleOrientations();


		for (size_t i = 0; i < particlesThatJustDied.size(); i++)
		{
			const entt::entity spawnedEntity = reg.CreateFromPrefab(*spawner.mPrefabToSpawn);
			TransformComponent* const transform = reg.TryGet<TransformComponent>(spawnedEntity);

			if (transform == nullptr)
			{
				continue;
			}

			const size_t particle = particlesThatJustDied[i];

			transform->SetWorldPosition(positions[particle]);
			transform->SetWorldOrientation(orientations[particle]);
			transform->SetWorldScale(scales[particle]);
		}
	}
}

Engine::MetaType Engine::ParticleSpawnPrefabOnDeathSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleSpawnPrefabOnDeathSystem>{}, "ParticleSpawnPrefabOnDeathSystem", MetaType::Base<System>{} };
}
