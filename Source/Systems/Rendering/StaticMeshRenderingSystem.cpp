#include "Precomp.h"
#include "Systems/Rendering/StaticMeshRenderingSystem.h"

#include "Components/MeshColorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Renderer.h"
#include "World/Registry.h"
#include "World/World.h"

void CE::StaticMeshRenderingSystem::Render(const World& world, RenderCommandQueue& commandQueue) const
{
	const auto renderView = [&commandQueue]<bool HasColor>(auto view)
		{
			for (const entt::entity entity : view)
			{
				const TransformComponent& transform = view.template get<TransformComponent>(entity);
				const StaticMeshComponent& staticMesh = view.template get<StaticMeshComponent>(entity);


				glm::vec4 multiplicativeColor{ 1.0f };
				glm::vec4 additiveColor{ 0.0f };

				if constexpr (HasColor)
				{
					const MeshColorComponent& color = view.template get<MeshColorComponent>(entity);
					multiplicativeColor = { color.mColorMultiplier, 1.0f };
					additiveColor = { color.mColorAddition, 0.0f };
				}

				Renderer::Get().AddStaticMesh(commandQueue,
					staticMesh.mStaticMesh,
					staticMesh.mMaterial,
					transform.GetWorldMatrix(),
					multiplicativeColor,
					additiveColor);
			}
		};

	renderView.operator()<false>(world.GetRegistry().View<TransformComponent, StaticMeshComponent>(entt::exclude_t<MeshColorComponent>{}));
	renderView.operator()<true>(world.GetRegistry().View<TransformComponent, StaticMeshComponent, MeshColorComponent>());
}

CE::MetaType CE::StaticMeshRenderingSystem::Reflect()
{
	return MetaType{ MetaType::T<StaticMeshRenderingSystem>{}, "StaticMeshRenderingSystem", MetaType::Base<System>{} };
}
