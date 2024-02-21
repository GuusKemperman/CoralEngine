#include "Precomp.h"
#include "Systems/Particles/ParticleColorSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleColorComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Utilities/Random.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::ParticleColorSystem::Update(World& world, [[maybe_unused]] float dt)
{
	const auto emitterView = world.GetRegistry().View<const ParticleEmitterComponent, ParticleColorComponent>();

	for (auto [entity, emitter, colorComponent] : emitterView.each())
	{
		colorComponent.mParticleColors.resize(emitter.GetNumOfParticles());
		Span<const size_t> newParticles = emitter.GetParticlesThatSpawnedDuringLastStep();

		for (size_t i = 0; i < newParticles.size(); i++)
		{
			const size_t particle = newParticles[i];

			const  LinearColor randomColor = LinearColor{ Math::lerp<glm::vec4>(colorComponent.mMinParticleColor, colorComponent.mMaxParticleColor, Random::Float()) };

			colorComponent.mParticleColors[particle] = randomColor;
		}
	}
}

Engine::MetaType Engine::ParticleColorSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleColorSystem>{}, "ParticleColorSystem", MetaType::Base<System>{} };
}
