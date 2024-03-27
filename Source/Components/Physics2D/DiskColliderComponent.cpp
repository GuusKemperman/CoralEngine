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

	metaType.AddFunc([](entt::entity owner)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			const auto transform = world->GetRegistry().TryGet<TransformedDiskColliderComponent>(owner);
			if (transform == nullptr)
			{
				LOG(LogPhysics, Error, "Entity {} passed to GetScaledRadius does not have a TransformedDiskColliderComponent.");
				return 0.0f;
			}
			return transform->mRadius;

		}, "GetScaledRadius", MetaFunc::ExplicitParams<
		entt::entity>{}, "Owner of Disk Collider").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

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

