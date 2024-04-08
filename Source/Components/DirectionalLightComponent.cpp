#include "Precomp.h"
#include "Components/DirectionalLightComponent.h"

#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::DirectionalLightComponent::OnDrawGizmos(World& world, entt::entity owner) const
{
	constexpr DebugCategory::Enum category = DebugCategory::Editor;
	constexpr float radius = 1.0f;
	constexpr float length = 4.0f;
	constexpr uint32 segments = 8;

	if (!DebugRenderer::IsCategoryVisible(category))
	{
		return;
	}

	const TransformComponent* const transform = world.GetRegistry().TryGet<TransformComponent>(owner);

	if (transform == nullptr)
	{
		return;
	}

	const glm::vec3 worldPos = transform->GetWorldPosition();

	const glm::vec3 drawToPosition = worldPos + transform->GetWorldForward() * length;

	DrawDebugCylinder(
		world,
		category,
		worldPos,
		drawToPosition,
		radius,
		segments,
		Colors::Yellow);
}

glm::mat4 CE::DirectionalLightComponent::GetShadowProjection() const
{
	return glm::ortho(-mShadowExtent, mShadowExtent, -mShadowExtent, mShadowExtent, -mShadowNearFar, mShadowNearFar);
}

glm::mat4 CE::DirectionalLightComponent::GetShadowView(const World& world, const TransformComponent& transform) const
{
	entt::entity cameraOwner = CameraComponent::GetSelected(world);
	const TransformComponent* const cameraTransform = world.GetRegistry().TryGet<TransformComponent>(cameraOwner);

	// Without the main camera the world won't be rendered anyway, so we just return a default matrix
	if (cameraTransform == nullptr)
	{
		return glm::mat4{1.0f};
	}

	glm::vec3 cameraPosition = cameraTransform->GetWorldPosition();

	// We try to follow the camera, that way when we are far away the shadow map will 
	// follow us as well and shadows will always work around the user.
	// This calculation is not ideal, since it just takes the center as the camera, but it is good enough for now,
	// as you don't notice the shadow map dissapearing when in top-down view.
	// TODO: A better approach would be to calculate the bounds of the view frustum and based on that calculate the view matrix
	return glm::lookAt(cameraPosition, cameraPosition + transform.GetWorldForward(), transform.GetWorldUp());
}

CE::MetaType CE::DirectionalLightComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DirectionalLightComponent>{}, "DirectionalLightComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&DirectionalLightComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&DirectionalLightComponent::mIntensity, "mIntensity").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&DirectionalLightComponent::mCastShadows, "mCastShadows").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&DirectionalLightComponent::mShadowExtent, "mShadowExtent").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&DirectionalLightComponent::mShadowNearFar, "mShadowNearFar").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&DirectionalLightComponent::mShadowBias, "mShadowBias").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&DirectionalLightComponent::mShadowSamples, "mShadowSamples").GetProperties().Add(Props::sIsScriptableTag);

#ifdef EDITOR
	BindEvent(metaType, sDrawGizmoEvent, &DirectionalLightComponent::OnDrawGizmos);
#endif // EDITOR

	ReflectComponentType<DirectionalLightComponent>(metaType);

	return metaType;
}
