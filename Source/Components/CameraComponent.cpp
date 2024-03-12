#include "Precomp.h"
#include "Components/CameraComponent.h" 

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/WorldRenderer.h"

void Engine::CameraComponent::UpdateView(const glm::vec3 position, const glm::vec3 forward, const glm::vec3 up, bool recalulateViewProjection)
{
	mView = glm::lookAt(position, position + forward, up);

	if (recalulateViewProjection)
	{
		RecalculateViewProjection();
	}
}

void Engine::CameraComponent::UpdateView(const TransformComponent& transform, bool recalulateViewProjection)
{
	UpdateView(transform.GetWorldPosition(), transform.GetWorldForward(), transform.GetWorldUp(), recalulateViewProjection);
}

void Engine::CameraComponent::UpdateProjection(const glm::vec2 viewportSize, bool recalculateViewProjection)
{
	UpdateProjection(viewportSize.x / viewportSize.y, recalculateViewProjection);
}

void Engine::CameraComponent::UpdateProjection(const float aspectRatio, bool recalculateViewProjection)
{
	mProjection = glm::perspective(mFOV, aspectRatio, mNear, mFar);

	if (recalculateViewProjection)
	{
		RecalculateViewProjection();
	}
}

void Engine::CameraComponent::RecalculateViewProjection()
{
	mViewProjection = mProjection * mView;
	mInvViewProjection = inverse(mViewProjection);
}

void Engine::CameraComponent::OnDeserialize(World& world, entt::entity owner, const BinaryGSONObject&)
{
	// This only gets called if we serialized something
	// during OnSerialize.
	// And we only serialize if we are the active camera.
	world.GetRenderer().SetMainCamera(owner);
}

void Engine::CameraComponent::OnSerialize(const World& world, entt::entity owner, BinaryGSONObject& serializeTo) const
{
	auto cam = world.GetRenderer().GetMainCamera();

	if (cam.has_value()
		&& cam->first == owner)
	{
		serializeTo.AddGSONMember("active");
	}
}

Engine::MetaType Engine::CameraComponent::Reflect()
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
	type.AddFunc(&CameraComponent::RecalculateViewProjection, "RecalculateViewProjection", "").GetProperties().Add(Props::sIsScriptableTag);

	BindEvent(type, sSerializeEvent, &CameraComponent::OnSerialize);
	BindEvent(type, sDeserializeEvent, &CameraComponent::OnDeserialize);

	ReflectComponentType<CameraComponent>(type);


	return type;
}
