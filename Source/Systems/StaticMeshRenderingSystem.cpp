#include "Precomp.h"
#include "Systems/StaticMeshRenderingSystem.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "xsr.hpp"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Assets/StaticMesh.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"

void Engine::StaticMeshRenderingSystem::Render(const World& world)
{
	const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();

    for (auto [entity, staticMeshComponent, transform] : view.each())
    {
        const auto& staticMesh = staticMeshComponent.mStaticMesh;
        const auto& material = staticMeshComponent.mMaterial;

        if (staticMesh == nullptr
            || material == nullptr)
        {
            continue;
        }

        const auto& texture = material->mBaseColorTexture;

        if (texture == nullptr)
        {
            continue;
        }

        render_mesh(value_ptr(transform.GetWorldMatrix()), staticMesh->GetHandle(), texture->GetHandle());
    }
}

Engine::MetaType Engine::StaticMeshRenderingSystem::Reflect()
{
    return MetaType{ MetaType::T<StaticMeshRenderingSystem>{}, "StaticMeshRenderingSystem", MetaType::Base<System>{} };
}
