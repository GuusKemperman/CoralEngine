#include "Precomp.h"
#include "Components/Physics2D/PolygonColliderComponent.h"

#include "Components/TransformComponent.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

CE::TransformedPolygon CE::PolygonColliderComponent::CreateTransformedCollider(const TransformComponent& transform) const
{
	const glm::mat4 worldMatrix = transform.GetWorldMatrix();

	PolygonPoints points{};
	points.resize(mPoints.size());

	for (uint32 i = 0; i < mPoints.size(); i++)
	{
		points[i] = worldMatrix * glm::vec4{ mPoints[i], 0.0f, 1.0f };
	}

	return points;
}

CE::MetaType CE::PolygonColliderComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<PolygonColliderComponent>{}, "PolygonColliderComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&PolygonColliderComponent::mPoints, "mPoints").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<PolygonColliderComponent>(metaType);

	return metaType;
}

CE::MetaType Reflector<CE::TransformedPolygon>::Reflect()
{
	using namespace CE;

	MetaType metaType = MetaType{ MetaType::T<TransformedPolygonColliderComponent>{}, "TransformedPolygonColliderComponent" };
	metaType.GetProperties().Add(Props::sNoInspectTag).Add(Props::sNoSerializeTag);

	ReflectComponentType<TransformedPolygonColliderComponent>(metaType);
	return metaType;
}
