#include "Precomp.h"
#include "Systems/Rendering/LightRenderingSystem.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TransformComponent.h"
#include "Core/Renderer.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"

void CE::LightRenderingSystem::Render(const World& world, RenderCommandQueue& commandQueue) const
{
	const Registry& reg = world.GetRegistry();

	for (const auto [entity, transform, directionalLight] : reg.View<TransformComponent, DirectionalLightComponent>().each())
	{
		Renderer::Get().AddDirectionalLight(commandQueue, transform.GetWorldForward(), directionalLight.mColor);
	}

	for (const auto [entity, transform, pointLight] : reg.View<TransformComponent, PointLightComponent>().each())
	{
		Renderer::Get().AddPointLight(commandQueue, transform.GetWorldPosition(), transform.GetWorldScaleUniform() * pointLight.mRange, pointLight.mColor);
	}
}

CE::MetaType CE::LightRenderingSystem::Reflect()
{
	return MetaType{ MetaType::T<LightRenderingSystem>{}, "LightRenderingSystem", MetaType::Base<System>{} };
}
