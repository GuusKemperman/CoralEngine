#include "Precomp.h"
#include "Systems/Rendering/ParticleRenderingSystem.h"

#include "Components/Particles/ParticleColorComponent.h"
#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleLightComponent.h"
#include "Components/Particles/ParticleMeshRendererComponent.h"
#include "Components/Particles/ParticleUtilities.h"
#include "Core/Renderer.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"

namespace
{
    template<typename T, typename ... Args>
    struct IsTypeInArgs {
        static constexpr bool value{ (std::is_same_v<T, Args> || ...) };
    };
}

void CE::ParticleRenderingSystem::Render(const World& world, RenderCommandQueue& commandQueue) const
{
    const auto renderParticles = [&commandQueue](const ParticleEmitterComponent& emitter, 
			const ParticleMeshRendererComponent* meshRenderer,
	        const ParticleColorComponent* colorComponent,
	        const ParticleLightComponent* lightComponent)
        {
            for (uint32 i = 0; i < emitter.GetNumOfParticles(); i++)
            {
                if (!emitter.IsParticleAlive(i))
                {
                    continue;
                }

                glm::vec4 multiplicativeColor{ 1.0f };
                glm::vec4 additiveColor{ 0.0f };

                if (colorComponent != nullptr)
                {
                    multiplicativeColor = colorComponent->mMultiplicativeColor.GetValue(emitter, i);
                    additiveColor = colorComponent->mAdditiveColor.GetValue(emitter, i);
                }

                if (meshRenderer != nullptr)
                {
                    const glm::mat4 particleTransform = emitter.GetParticleMatrixWorld(i);

                    Renderer::Get().AddStaticMesh(commandQueue, 
                        meshRenderer->mParticleMesh, 
                        meshRenderer->mParticleMaterial,
                        particleTransform, 
                        multiplicativeColor, 
                        additiveColor);
                }

                if (lightComponent != nullptr)
                {
                    const glm::vec3 particlePos = emitter.GetParticlePositionWorld(i);
                    Renderer::Get().AddPointLight(commandQueue, particlePos, lightComponent->mRadius.GetValue(emitter, i), additiveColor * multiplicativeColor);
                }
            }
        };

    const auto renderView = [&renderParticles, &world]<typename... Included>(const auto excluded)
	    {
			const auto view = world.GetRegistry().View<Included...>(excluded);
	        for (const entt::entity entity : view)
	        {
	            const ParticleEmitterComponent& emitter = view.template get<ParticleEmitterComponent>(entity);

                const auto getIfInView = [&view, &entity]<typename T>() -> const T*
	                {
                        if constexpr (IsTypeInArgs<T, Included...>::value)
                        {
                            return &view.template get<T>(entity);
                        }
                        else
                        {
                            return nullptr;
                        }
	                };

                const ParticleMeshRendererComponent* meshRenderer = getIfInView.template operator()<ParticleMeshRendererComponent>();
                const ParticleColorComponent* colorComponent = getIfInView.template operator()<ParticleColorComponent>();
                const ParticleLightComponent* lightComponent = getIfInView.template operator()<ParticleLightComponent>();

                renderParticles(emitter, meshRenderer, colorComponent, lightComponent);
	        }
	    };

    // All possible combinations
    renderView.operator()<ParticleEmitterComponent, ParticleMeshRendererComponent, ParticleColorComponent, ParticleLightComponent>(entt::exclude_t{});
    renderView.operator()<ParticleEmitterComponent, ParticleMeshRendererComponent, ParticleLightComponent>(entt::exclude_t<ParticleColorComponent>{});
    renderView.operator()<ParticleEmitterComponent, ParticleMeshRendererComponent, ParticleColorComponent>(entt::exclude_t<ParticleLightComponent>{});
    renderView.operator()<ParticleEmitterComponent, ParticleMeshRendererComponent>(entt::exclude_t<ParticleColorComponent, ParticleLightComponent>{});
    renderView.operator()<ParticleEmitterComponent, ParticleColorComponent, ParticleLightComponent>(entt::exclude_t<ParticleMeshRendererComponent>{});
    renderView.operator()<ParticleEmitterComponent, ParticleLightComponent>(entt::exclude_t<ParticleMeshRendererComponent, ParticleColorComponent>{});
}

CE::MetaType CE::ParticleRenderingSystem::Reflect()
{
	return MetaType{ MetaType::T<ParticleRenderingSystem>{}, "ParticleRenderingSystem", MetaType::Base<System>{} };
}
