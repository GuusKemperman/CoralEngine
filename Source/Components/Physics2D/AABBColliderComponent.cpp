#include "Precomp.h"
#include "Components/Physics2D/AABBColliderComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::TransformedAABB CE::AABBColliderComponent::CreateTransformedCollider(const TransformComponent& transform) const
{
	const glm::vec2 worldPosition = transform.GetWorldPosition2D();
	const glm::vec2 scale = transform.GetWorldScale2D();
	const glm::vec2 scaledHalfExtends = mHalfExtends * scale;
	return { worldPosition - scaledHalfExtends, worldPosition + scaledHalfExtends };
}

CE::MetaType CE::AABBColliderComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AABBColliderComponent>{}, "AABBColliderComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Set(Props::sOldNames, "AABBColliderComponnet");

	metaType.AddField(&AABBColliderComponent::mHalfExtends, "mHalfExtends").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<AABBColliderComponent>(metaType);

	return metaType;
}

CE::MetaType Reflector<CE::TransformedAABB>::Reflect()
{
	using namespace CE;

	MetaType metaType = MetaType{ MetaType::T<TransformedAABBColliderComponent>{}, "TransformedAABBColliderComponent" };
	metaType.GetProperties().Add(Props::sNoInspectTag).Add(Props::sNoSerializeTag);

	ReflectComponentType<TransformedAABBColliderComponent>(metaType);
	return metaType;
}
