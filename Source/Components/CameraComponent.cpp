#include "Precomp.h"
#include "Components/CameraComponent.h" 

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/WorldViewport.h"

void CE::CameraComponent::UpdateView(const glm::vec3 position, const glm::vec3 forward, const glm::vec3 up, bool recalulateViewProjection)
{
	mView = glm::lookAt(position, position + forward, up);
	mPosition = position;

	if (recalulateViewProjection)
	{
		RecalculateViewProjection();
	}
}

void CE::CameraComponent::UpdateView(const TransformComponent& transform, bool recalulateViewProjection)
{
	UpdateView(transform.GetWorldPosition(), transform.GetWorldForward(), transform.GetWorldUp(), recalulateViewProjection);
	mPosition = transform.GetWorldPosition();

}

void CE::CameraComponent::UpdateProjection(const glm::vec2 viewportSize, bool recalculateViewProjection)
{
	UpdateProjection(viewportSize.x / viewportSize.y, recalculateViewProjection);
	mViewportSize = viewportSize;
}

void CE::CameraComponent::UpdateProjection(const float aspectRatio, bool recalculateViewProjection)
{
	mProjection = glm::perspective(mFOV, aspectRatio, mNear, mFar);

	// Calculating the orthographic projection is a matter to discuss and look into further, 
	// since it may not work as well on different resolutions
	const float halfAspectRatio = aspectRatio * 0.5f;
	mOrthographicProjection = glm::ortho(-halfAspectRatio, halfAspectRatio, -halfAspectRatio, halfAspectRatio, -1.0f, 1.0f);

	if (recalculateViewProjection)
	{
		RecalculateViewProjection();
	}
}

void CE::CameraComponent::RecalculateViewProjection()
{
	mViewProjection = mProjection * mView;
	mInvViewProjection = inverse(mViewProjection);
}

entt::entity CE::CameraComponent::GetSelected(const World& world)
{
	World& mutWorld = const_cast<World&>(world);
	Registry& reg = mutWorld.GetRegistry();
	entt::entity selectedEntity = reg.View<CameraComponent, CameraSelectedTag>().front();

	if (selectedEntity == entt::null)
	{
		Deselect(mutWorld);
		selectedEntity = reg.View<CameraComponent>().front();

		if (selectedEntity != entt::null)
		{
			Select(mutWorld, selectedEntity);
		}
	}

	return selectedEntity;
}

bool CE::CameraComponent::IsSelected(const World& world, entt::entity cameraOwner)
{
	return world.GetRegistry().HasComponent<CameraSelectedTag>(cameraOwner);
}

void CE::CameraComponent::Select(World& world, entt::entity cameraOwner)
{
	Deselect(world);
	world.GetRegistry().AddComponent<CameraSelectedTag>(cameraOwner);
}

void CE::CameraComponent::Deselect(World& world)
{
	const auto view = world.GetRegistry().View<CameraSelectedTag>();

	std::vector<entt::entity> entitiesWithTag{};
	entitiesWithTag.reserve(view.size());

	for (auto [entity] : view.each())
	{
		entitiesWithTag.emplace_back(entity);

	}

	for (const entt::entity entity : entitiesWithTag)
	{
		world.GetRegistry().RemoveComponent<CameraSelectedTag>(entity);
	}
}

CE::MetaType CE::CameraComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<CameraComponent>{}, "CameraComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&CameraComponent::mFar, "mFar").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&CameraComponent::mNear, "mNear").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&CameraComponent::mFOV, "mFOV").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&CameraComponent::GetViewProjection, "GetViewProjection", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&CameraComponent::GetView, "GetView", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&CameraComponent::GetProjection, "GetProjection", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&CameraComponent::GetOrthographicProjection, "GetOrthographicProjection", "").GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(&CameraComponent::RecalculateViewProjection, "RecalculateViewProjection", "").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<CameraComponent>(type);

	return type;
}

CE::MetaType CE::CameraSelectedTag::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<CameraSelectedTag>{}, "CameraSelectedTag" };
	metaType.GetProperties().Add(Props::sNoInspectTag);

	ReflectComponentType<CameraSelectedTag>(metaType);

	return metaType;
}
