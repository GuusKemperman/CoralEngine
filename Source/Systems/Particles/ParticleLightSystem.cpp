#include "Precomp.h"
#include "Systems/Particles/ParticleLightSystem.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleLightComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleUtilities.h"
#include "Meta/MetaType.h"

void CE::ParticleLightSystem::Update(World& world, [[maybe_unused]] float dt)
{
	const auto emitterView = world.GetRegistry().View<const ParticleEmitterComponent, ParticleLightComponent>();

	for (auto [entity, emitter, lightComponent] : emitterView.each())
	{
		lightComponent.mIntensity.SetInitialValuesOfNewParticles(emitter);
		lightComponent.mRadius.SetInitialValuesOfNewParticles(emitter);
	}
}

CE::MetaType CE::ParticleLightSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleLightSystem>{}, "ParticleLightSystem", MetaType::Base<System>{} };
}
