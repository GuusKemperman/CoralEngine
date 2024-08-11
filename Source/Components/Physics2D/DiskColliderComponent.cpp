#include "Precomp.h"
#include "Components/Physics2D/DiskColliderComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::TransformedDisk CE::DiskColliderComponent::CreateTransformedCollider(const TransformComponent& transform) const
{
	const glm::vec2 scale = transform.GetWorldScale();
	const float scaleUniform = .5f * (scale.x + scale.y);
	return { transform.GetWorldPosition(), scaleUniform * mRadius };
}

CE::MetaType CE::DiskColliderComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DiskColliderComponent>{}, "DiskColliderComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&DiskColliderComponent::mRadius, "mRadius").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<DiskColliderComponent>(metaType);

	return metaType;
}

CE::MetaType Reflector<CE::TransformedDisk>::Reflect()
{
	using namespace CE;

	MetaType metaType = MetaType{ MetaType::T<TransformedDiskColliderComponent>{}, "TransformedDiskColliderComponent" };
	metaType.GetProperties().Add(Props::sNoInspectTag).Add(Props::sNoSerializeTag);

	ReflectComponentType<TransformedDiskColliderComponent>(metaType);
	return metaType;
}

