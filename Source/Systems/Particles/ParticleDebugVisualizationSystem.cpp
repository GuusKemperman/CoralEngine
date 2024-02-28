#include "Precomp.h"
#include "Systems/Particles/ParticleDebugVisualizationSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "World/WorldRenderer.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticlePhysicsComponent.h"
#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void Engine::ParticleDebugVisualizationSystem::Render(const World& world)
{
	const WorldRenderer& worldRenderer = world.GetRenderer();

	if ((worldRenderer.GetDebugCategoryFlags() & DebugCategory::Particles) == 0)
	{
		return;
	}

	const Registry& reg = world.GetRegistry();

	auto physicsView = reg.View<const ParticlePhysicsComponent, const ParticleEmitterComponent>();

	for (auto [entity, physics, emitter] : physicsView.each())
	{
		const uint32 numOfParticles = emitter.GetNumOfParticles();

		const auto& positions = emitter.GetParticlePositions();
		const auto& orientations = emitter.GetParticleOrientations();
		const auto& velocities = physics.GetLinearVelocities();

		glm::vec3 boundingBoxMin{INFINITY};
		glm::vec3 boundingBoxMax{-INFINITY};

		for (uint32 i = 0; i < numOfParticles; i++)
		{
			if (!emitter.IsParticleAlive(i))
			{
				continue;
			}

			const glm::vec3 particlePos = positions[i];
			boundingBoxMax = (glm::max)(boundingBoxMax, particlePos);
			boundingBoxMin = (glm::min)(boundingBoxMin, particlePos);

			{ // Draw velocity
				const glm::vec3 lineEnd = particlePos + velocities[i];
				constexpr glm::vec4 color = glm::vec4{ 1.0f };

				worldRenderer.AddLine(DebugCategory::Particles, particlePos, lineEnd, color);
			}

			const glm::vec3 forward = Math::RotateVector(sForward, orientations[i]);
			const glm::vec3 right = Math::RotateVector(sRight, orientations[i]);
			const glm::vec3 up = cross(forward, -right);

			worldRenderer.AddLine(DebugCategory::Particles, particlePos, particlePos + forward, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f});
			worldRenderer.AddLine(DebugCategory::Particles, particlePos, particlePos + right, glm::vec4{0.0f, 1.0f, 0.0f, 1.0f});
			worldRenderer.AddLine(DebugCategory::Particles, particlePos, particlePos + up, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
		}

		if (boundingBoxMin.x != INFINITY)
		{
			const glm::vec3 halfExtends = (boundingBoxMax - boundingBoxMin) * .5f;
			const glm::vec3 centre = boundingBoxMin + halfExtends;
			worldRenderer.AddBox(DebugCategory::Particles, centre, halfExtends, glm::vec4{ 1.0f, 1.0f, 0.0f, 1.0f });
		}
	}
}

Engine::MetaType Engine::ParticleDebugVisualizationSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleDebugVisualizationSystem>{}, "ParticleDebugVisualizationSystem", MetaType::Base<System>{} };
}
