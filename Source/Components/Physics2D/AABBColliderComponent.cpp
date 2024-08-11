#include "Precomp.h"
#include "Components/Physics2D/AABBColliderComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Math.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::TransformedAABB CE::AABBColliderComponent::CreateTransformedCollider(const TransformComponent& transform) const
{
	const glm::vec2 worldPosition = transform.GetWorldPosition();
	const glm::vec2 scale = transform.GetWorldScale();
	const glm::vec2 scaledHalfExtends = mHalfExtends * scale;

	const glm::vec2 vertices[4] =
	{
		-scaledHalfExtends,
		{ -scaledHalfExtends.x, scaledHalfExtends.y },
		{ scaledHalfExtends.x, -scaledHalfExtends.y },
		scaledHalfExtends
	};

	TransformedAABBColliderComponent rotatedAABB{ glm::vec2{ std::numeric_limits<float>::infinity() }, glm::vec2{ -std::numeric_limits<float>::infinity() } };
	const glm::quat orientation = transform.GetWorldOrientation();

	for (int i = 0; i < 4; i++)
	{
		const glm::vec2 transformedVertex = To2D(Math::RotateVector(To3D(vertices[i], 0.0f), orientation));

		rotatedAABB.mMin = glm::min(rotatedAABB.mMin, transformedVertex);
		rotatedAABB.mMax = glm::max(rotatedAABB.mMax, transformedVertex);
	}

	rotatedAABB.mMin += worldPosition;
	rotatedAABB.mMax += worldPosition;

	return rotatedAABB;
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
