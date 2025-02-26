#include "Precomp.h"
#include "Systems/UpdateCameraMatricesSystem.h"

#include "World/World.h"
#include "World/Registry.h"
#include "World/WorldViewport.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h" 
#include "Meta/MetaType.h"

void CE::UpdateCameraMatricesSystem::Update(World& world, [[maybe_unused]] float dt)
{
	UpdateMatrices(world);
}

void CE::UpdateCameraMatricesSystem::Render(const World& world, [[maybe_unused]] RenderCommandQueue& commandQueue) const
{
	// In some rare cases, we will call Render without ever calling Tick.
	// For example when rendering the thumbnails.
	// So in Render, we also update the matrices.
	UpdateMatrices(const_cast<World&>(world));
}

void CE::UpdateCameraMatricesSystem::UpdateMatrices(World& world)
{
	const glm::vec2 viewportSize = world.GetViewport().GetViewportSize();
	const float aspectRatio = viewportSize.x / viewportSize.y;

	const auto viewWithTransform = world.GetRegistry().View<CameraComponent, const TransformComponent>();
	for (auto [entity, camera, transform] : viewWithTransform.each())
	{
		const glm::vec3 worldPos = transform.GetWorldPosition();
		camera.mView = glm::lookAt(worldPos, worldPos + transform.GetWorldForward(), transform.GetWorldUp());

		camera.mProjection = camera.mIsOrthoGraphic ?
			glm::ortho(-aspectRatio * camera.mOrthoGraphicSize, aspectRatio * camera.mOrthoGraphicSize, -camera.mOrthoGraphicSize, camera.mOrthoGraphicSize, camera.mNear, camera.mFar) :
			glm::perspective(camera.mFOV, aspectRatio, camera.mNear, camera.mFar);

		camera.mViewProjection = camera.mProjection * camera.mView;
	}
}

CE::MetaType CE::UpdateCameraMatricesSystem::Reflect()
{
	return MetaType{ MetaType::T<UpdateCameraMatricesSystem>{}, "UpdateCameraMatricesSystem", MetaType::Base<System>{} };
}
