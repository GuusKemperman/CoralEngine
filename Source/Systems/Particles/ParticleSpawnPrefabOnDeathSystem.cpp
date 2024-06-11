#include "Precomp.h"
#include "Systems/Particles/ParticleSpawnPrefabOnDeathSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleSpawnPrefabOnDeathComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleProperty.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"

void CE::ParticleSpawnPrefabOnDeathSystem::Update(World& world, [[maybe_unused]] float dt)
{
	Registry& reg = world.GetRegistry();

	const auto view = reg.View<const ParticleEmitterComponent, const ParticleSpawnPrefabOnDeathComponent>();

	for (auto [entity, emitter, spawner] : view.each())
	{
		if (spawner.mPrefabToSpawn == nullptr)
		{
			continue;
		}

		for (const uint32 particle : emitter.GetParticlesThatDiedDuringLastStep())
		{
			const entt::entity spawnedEntity = reg.CreateFromPrefab(*spawner.mPrefabToSpawn);
			TransformComponent* const transform = reg.TryGet<TransformComponent>(spawnedEntity);

			if (transform == nullptr)
			{
				continue;
			}

			transform->SetWorldPosition(emitter.GetParticlePositionWorld(particle));
			transform->SetWorldOrientation(emitter.GetParticleOrientationWorld(particle));
			transform->SetWorldScale(emitter.mScale.GetValue(emitter, particle));
		}
	}
}

CE::MetaType CE::ParticleSpawnPrefabOnDeathSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleSpawnPrefabOnDeathSystem>{}, "ParticleSpawnPrefabOnDeathSystem", MetaType::Base<System>{} };
}