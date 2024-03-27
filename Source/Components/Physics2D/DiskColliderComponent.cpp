#include "Precomp.h"
#include "Components/Physics2D/DiskColliderComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::TransformedDisk Engine::DiskColliderComponent::CreateTransformedCollider(const TransformComponent& transform) const
{
	return { transform.GetWorldPosition2D(), transform.GetWorldScaleUniform2D() * mRadius };
}

Engine::MetaType Engine::DiskColliderComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DiskColliderComponent>{}, "DiskColliderComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&DiskColliderComponent::mRadius, "mRadius").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<DiskColliderComponent>(metaType);

	return metaType;
}

Engine::MetaType Reflector<Engine::TransformedDisk>::Reflect()
{
	using namespace Engine;

	MetaType metaType = MetaType{ MetaType::T<TransformedDiskColliderComponent>{}, "TransformedDiskColliderComponent" };
	metaType.GetProperties().Add(Props::sNoInspectTag).Add(Props::sNoSerializeTag);

	ReflectComponentType<TransformedDiskColliderComponent>(metaType);
	return metaType;
}

