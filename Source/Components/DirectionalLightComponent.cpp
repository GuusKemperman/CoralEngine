#include "Precomp.h"
#include "Components/DirectionalLightComponent.h"

#include "Components/TransformComponent.h"
#include "Rendering/DrawDebugHelpers.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::DirectionalLightComponent::OnDrawGizmos(World& world, entt::entity owner) const
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

Engine::MetaType Engine::DirectionalLightComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DirectionalLightComponent>{}, "DirectionalLightComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&DirectionalLightComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&DirectionalLightComponent::mIntensity, "mIntensity").GetProperties().Add(Props::sIsScriptableTag);

#ifdef EDITOR
	BindEvent(metaType, sDrawGizmoEvent, &DirectionalLightComponent::OnDrawGizmos);
#endif // EDITOR

	ReflectComponentType<DirectionalLightComponent>(metaType);

	return metaType;
}
