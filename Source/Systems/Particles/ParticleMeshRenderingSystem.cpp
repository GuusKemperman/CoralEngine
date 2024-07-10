#include "Precomp.h"
#include "Systems/Particles/ParticleMeshRenderingSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "Components/Particles/ParticleMeshRendererComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Particles/ParticleColorComponent.h"
#include "Components/Particles/ParticleColorOverTimeComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Assets/StaticMesh.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"

bool AreAnyVisible(const Engine::ParticleEmitterComponent& emitter, const Engine::ParticleMeshRendererComponent& meshRenderer)
{
	return emitter.IsPlaying()
		&& meshRenderer.mParticleMesh != nullptr
		&& meshRenderer.mParticleMaterial != nullptr
		&& meshRenderer.mParticleMaterial->mBaseColorTexture != nullptr;
}

void Engine::ParticleMeshRenderingSystem::Render([[maybe_unused]] const World& world)
{
	const Registry& reg = world.GetRegistry();

	SimpleRender(reg);
	ColorRender(reg);
	ColorOverTimeRender(reg);
}

void Engine::ParticleMeshRenderingSystem::SimpleRender(const Registry& reg)
{
	const auto view = reg.View<const ParticleEmitterComponent, const ParticleMeshRendererComponent>(entt::exclude<ParticleColorComponent>);

	for (auto [entity, emitter, meshRenderer] : view.each())
	{
		if (!AreAnyVisible(emitter, meshRenderer))
		{
			continue;
		}

		const uint32 numOfParticles = emitter.GetNumOfParticles();

		const xsr::mesh_handle meshHandle = meshRenderer.mParticleMesh->GetHandle();
		const xsr::texture_handle textureHandle = meshRenderer.mParticleMaterial->mBaseColorTexture->GetHandle();

		Span<const glm::vec3> positions = emitter.GetParticlePositions();
		Span<const glm::vec3> sizes = emitter.GetParticleSizes();
		Span<const glm::quat> orientations = emitter.GetParticleOrientations();

		for (uint32 i = 0; i < numOfParticles; i++)
		{
			if (emitter.IsParticleAlive(i))
			{
				const glm::mat4 mat = TransformComponent::ToMatrix(positions[i], sizes[i], orientations[i]);
				render_mesh(value_ptr(mat), meshHandle, textureHandle, value_ptr(glm::vec4{ 1.0f }));
			}
		}
	}
}
void Engine::ParticleMeshRenderingSystem::ColorRender(const Registry& reg)
{
	const auto view = reg.View<const ParticleEmitterComponent, const ParticleMeshRendererComponent, const ParticleColorComponent>(entt::exclude<ParticleColorOverTimeComponent>);

	for (auto [entity, emitter, meshRenderer, colorComponent] : view.each())
	{
		if (!AreAnyVisible(emitter, meshRenderer))
		{
			continue;
		}

		const size_t numOfParticles = emitter.GetNumOfParticles();

		const xsr::mesh_handle meshHandle = meshRenderer.mParticleMesh->GetHandle();
		const xsr::texture_handle textureHandle = meshRenderer.mParticleMaterial->mBaseColorTexture->GetHandle();

		Span<const glm::vec3> positions = emitter.GetParticlePositions();
		Span<const glm::vec3> sizes = emitter.GetParticleSizes();
		Span<const glm::quat> orientations = emitter.GetParticleOrientations();
		Span<const LinearColor> colors = colorComponent.GetColors();

		for (uint32 i = 0; i < numOfParticles; i++)
		{
			if (emitter.IsParticleAlive(i))
			{
				const glm::mat4 mat = TransformComponent::ToMatrix(positions[i], sizes[i], orientations[i]);
				render_mesh(value_ptr(mat), meshHandle, textureHandle, &colors[i][0]);
			}
		}
	}
}
void Engine::ParticleMeshRenderingSystem::ColorOverTimeRender(const Registry& reg)
{
	const auto view = reg.View<const ParticleEmitterComponent, const ParticleMeshRendererComponent, const ParticleColorComponent, const ParticleColorOverTimeComponent>();

	for (auto [entity, emitter, meshRenderer, colorComponent, colorOverTime] : view.each())
	{
		if (!AreAnyVisible(emitter, meshRenderer))
		{
			continue;
		}

		const uint32 numOfParticles = emitter.GetNumOfParticles();

		const xsr::mesh_handle meshHandle = meshRenderer.mParticleMesh->GetHandle();
		const xsr::texture_handle textureHandle = meshRenderer.mParticleMaterial->mBaseColorTexture->GetHandle();

		Span<const float> lifeTimes = emitter.GetParticleLifeTimesAsPercentage();
		Span<const glm::vec3> positions = emitter.GetParticlePositions();
		Span<const glm::vec3> sizes = emitter.GetParticleSizes();
		Span<const glm::quat> orientations = emitter.GetParticleOrientations();
		Span<const LinearColor> colors = colorComponent.GetColors();
		const ColorGradient& gradient = colorOverTime.mGradient;

		for (uint32 i = 0; i < numOfParticles; i++)
		{
			if (emitter.IsParticleAlive(i))
			{
				const glm::mat4 mat = TransformComponent::ToMatrix(positions[i], sizes[i], orientations[i]);
				const LinearColor color = colors[i] * gradient.GetColorAt(lifeTimes[i]);
				render_mesh(value_ptr(mat), meshHandle, textureHandle, &color[0]);
			}
		}
	}
}

Engine::MetaType Engine::ParticleMeshRenderingSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleMeshRenderingSystem>{}, "ParticleMeshRenderingSystem", MetaType::Base<System>{} };
}
