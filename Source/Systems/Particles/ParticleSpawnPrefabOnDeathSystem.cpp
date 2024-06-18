#include "Precomp.h"
#include "Systems/Particles/ParticleSpawnPrefabOnDeathSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleSpawnPrefabOnDeathComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleUtilities.h"
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

			if (spawner.mKeepPosition)
			{
				transform->SetWorldPosition(emitter.GetParticlePositionWorld(particle));
			}

			if (spawner.mKeepScale)
			{
				transform->SetWorldScale(emitter.mScale.GetValue(emitter, particle));
			}

			if (spawner.mKeepOrientation)
			{
				transform->SetWorldOrientation(emitter.GetParticleOrientationWorld(particle));
			}
		}
	}
}

CE::MetaType CE::ParticleSpawnPrefabOnDeathSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleSpawnPrefabOnDeathSystem>{}, "ParticleSpawnPrefabOnDeathSystem", MetaType::Base<System>{} };
}