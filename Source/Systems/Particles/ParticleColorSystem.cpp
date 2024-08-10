#include "Precomp.h"
#include "Systems/Particles/ParticleColorSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleColorComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleUtilities.h"
#include "Meta/MetaType.h"

void CE::ParticleColorSystem::Update(World& world, [[maybe_unused]] float dt)
{
	const auto emitterView = world.GetRegistry().View<const ParticleEmitterComponent, ParticleColorComponent>();

	for (auto [entity, emitter, colorComponent] : emitterView.each())
	{
		colorComponent.mMultiplicativeColor.SetInitialValuesOfNewParticles(emitter);
		colorComponent.mAdditiveColor.SetInitialValuesOfNewParticles(emitter);
	}
}

CE::MetaType CE::ParticleColorSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleColorSystem>{}, "ParticleColorSystem", MetaType::Base<System>{} };
}
