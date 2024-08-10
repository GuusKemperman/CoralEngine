#include "Precomp.h"
#include "Components/DirectionalLightComponent.h"

#include "Components/TransformComponent.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::DirectionalLightComponent::OnDrawGizmos(World& world, entt::entity owner) const
{
	static constexpr DebugDraw::Enum category = DebugDraw::Editor;
	static constexpr float radius = 1.0f;
	static constexpr float length = 4.0f;

	if (!IsDebugDrawCategoryVisible(category))
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

	AddDebugCylinder(world.GetRenderCommandQueue(),
		category,
		worldPos,
		drawToPosition,
		radius,
		{ 1.0f, 1.0f, 0.0f, 1.0f });
}

CE::MetaType CE::DirectionalLightComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DirectionalLightComponent>{}, "DirectionalLightComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&DirectionalLightComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);

#ifdef EDITOR
	BindEvent(metaType, sOnDrawGizmo, &DirectionalLightComponent::OnDrawGizmos);
#endif // EDITOR

	ReflectComponentType<DirectionalLightComponent>(metaType);

	return metaType;
}
