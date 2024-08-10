#include "Precomp.h"
#include "Components/PointLightComponent.h"

#include "Components/TransformComponent.h"
#include "Utilities/DrawDebugHelpers.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::PointLightComponent::OnDrawGizmos(World& world, entt::entity owner) const
{
	static constexpr DebugDraw::Enum category = DebugDraw::Editor;

	if (!IsDebugDrawCategoryVisible(category))
	{
		return;
	}

	const TransformComponent* const transform = world.GetRegistry().TryGet<TransformComponent>(owner);

	if (transform == nullptr)
	{
		return;
	}

	AddDebugSphere(world.GetRenderCommandQueue(),
		category,
		transform->GetWorldPosition(),
		transform->GetWorldScaleUniform() * mRange,
		{ 1.0f, 1.0f, 0.0f, 1.0f });
}

CE::MetaType CE::PointLightComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PointLightComponent>{}, "PointLightComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&PointLightComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&PointLightComponent::mRange, "mRange").GetProperties().Add(Props::sIsScriptableTag);

#ifdef EDITOR
	BindEvent(metaType, sOnDrawGizmo, &PointLightComponent::OnDrawGizmos);
#endif // EDITOR

	ReflectComponentType<PointLightComponent>(metaType);

	return metaType;
}
