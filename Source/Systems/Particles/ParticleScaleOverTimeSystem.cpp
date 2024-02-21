#include "Precomp.h"
#include "Systems/Particles/ParticleScaleOverTimeSystem.h"

#include "Components/Particles/ParticleScaleOverTimeComponent.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::ParticleScaleOverTimeSystem::Update(World& world, [[maybe_unused]] float dt)
{
	const auto emitterView = world.GetRegistry().View<ParticleEmitterComponent, ParticleScaleOverTimeComponent>();

	for (auto [entity, emitter, scaleOverTimeComponent] : emitterView.each())
	{
		if (emitter.IsPaused())
		{
			continue;
		}

		const size_t numOfParticles = emitter.GetNumOfParticles();

		scaleOverTimeComponent.mParticleInitialScales.resize(emitter.GetNumOfParticles());
		Span<const size_t> newParticles = emitter.GetParticlesThatSpawnedDuringLastStep();
		Span<glm::vec3> particleSizes = emitter.GetParticleSizes();

		for (size_t i = 0; i < newParticles.size(); i++)
		{
			const size_t particle = newParticles[i];
			scaleOverTimeComponent.mParticleInitialScales[particle] = particleSizes[particle];
		}

		Span<const float> particleLifeTime = emitter.GetParticleLifeTimesAsPercentage();

		for (size_t i = 0; i < numOfParticles; i++)
		{
			// Benchmarked, and was slightly slower
			//if (emitter.IsParticleAlive(i))
			{
				particleSizes[i] = scaleOverTimeComponent.mParticleInitialScales[i] * scaleOverTimeComponent.mScaleMultiplierOverParticleLifeTime.GetValueAt(glm::clamp(particleLifeTime[i], 0.0f, 1.0f));
			}
		}
	}
}

Engine::MetaType Engine::ParticleScaleOverTimeSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleScaleOverTimeSystem>{}, "ParticleScaleOverTimeSystem", MetaType::Base<System>{} };
}
