#include "Precomp.h"
#include "Systems/UpdateCameraMatricesSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "World/WorldViewport.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h" 
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

void CE::UpdateCameraMatricesSystem::Update(World& world, float)
{
	const glm::vec2 viewportSize = world.GetViewport().GetViewportSize();
	const float aspectRatio = viewportSize.x / viewportSize.y;

	const auto viewWithTransform = world.GetRegistry().View<CameraComponent, const TransformComponent>();
	for (auto [entity, camera, transform] : viewWithTransform.each())
	{
		camera.UpdateView(transform, false);
		camera.UpdateProjection(aspectRatio, true);
	}

	const auto viewWithoutTransform = world.GetRegistry().View<CameraComponent>(entt::exclude<TransformComponent>);
	for (auto [entity, camera] : viewWithoutTransform.each())
	{
		camera.UpdateView(glm::vec3{0.0f}, sForward, sUp, false);
		camera.UpdateProjection(aspectRatio, true);
	}
}

CE::MetaType CE::UpdateCameraMatricesSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdateCameraMatricesSystem>{}, "UpdateCameraMatricesSystem", MetaType::Base<System>{} };
}
