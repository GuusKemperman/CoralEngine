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

	metaType.AddFunc([](entt::entity owner)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const auto transform = world->GetRegistry().TryGet<TransformComponent>(owner);
			if (transform == nullptr)
			{
				LOG(LogPhysics, Error, "Entity {} passed to GetScaledRadius does not have a TransformComponent.", entt::to_integral(owner));
				return 0.0f;
			}
			const auto collider = world->GetRegistry().TryGet<DiskColliderComponent>(owner);
			if (transform == nullptr)
			{
				LOG(LogPhysics, Error, "Entity {} passed to GetScaledRadius does not have a DiskColliderComponent.", entt::to_integral(owner));
				return 0.0f;
			}

			return collider->CreateTransformedCollider(*transform).mRadius;
		}, "GetScaledRadius", MetaFunc::ExplicitParams<
		entt::entity>{}, "Owner of Disk Collider").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

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

