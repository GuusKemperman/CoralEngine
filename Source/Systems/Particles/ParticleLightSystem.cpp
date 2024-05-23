#include "Precomp.h"
#include "Systems/Particles/ParticleLightSystem.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleColorComponent.h"
#include "Components/Particles/ParticleLightComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Utilities/Random.h"
#include "Meta/MetaType.h"
#include "Utilities/Math.h"

void CE::ParticleLightSystem::Update(World& world, [[maybe_unused]] float dt)
{
	const auto emitterView = world.GetRegistry().View<const ParticleEmitterComponent, ParticleLightComponent>();

	for (auto [entity, emitter, lightComponent] : emitterView.each())
	{
		lightComponent.mParticleLightIntensities.resize(emitter.GetNumOfParticles());
		Span<const size_t> newParticles = emitter.GetParticlesThatSpawnedDuringLastStep();

		for (size_t i = 0; i < newParticles.size(); i++)
		{
			const size_t particle = newParticles[i];

			const float randomIntensity = float{ Math::lerp<float>(lightComponent.mMinLightIntensity, lightComponent.mMaxLightIntensity, Random::Value<float>()) };

			lightComponent.mParticleLightIntensities[particle] = randomIntensity;
		}
	}

}

CE::MetaType CE::ParticleLightSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleLightSystem>{}, "ParticleLightSystem", MetaType::Base<System>{} };
}
