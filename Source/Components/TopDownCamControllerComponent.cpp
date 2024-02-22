#include "Precomp.h"
#include "Components/TopDownCamControllerComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::TopDownCamControllerComponent::ApplyTranslation(TransformComponent& transform,  const glm::vec3& target, const glm::vec2& cursorDistanceFromScreenCenter) const
{
	glm::vec3 totalTranslation = target;
	totalTranslation[Axis::Up] += mOffsetHeight;
	totalTranslation[Axis::Right] += cursorDistanceFromScreenCenter.x * mCursorOffsetFactor + glm::cos(glm::radians(mAngle)) * mOffset;
	totalTranslation[Axis::Forward] += -cursorDistanceFromScreenCenter.y * mCursorOffsetFactor + glm::sin(glm::radians(mAngle)) * mOffset;

	transform.SetWorldPosition(totalTranslation);
}

void Engine::TopDownCamControllerComponent::UpdateRotation(TransformComponent& transform, const glm::vec3& target)
{
	const glm::vec3 direction = glm::normalize(target - transform.GetWorldPosition());

	transform.SetWorldOrientation(glm::quatLookAtLH(direction, sUp));
}

void Engine::TopDownCamControllerComponent::AdjustZoom(const float scrollDelta)
{
	float multiplier = 1.0f + scrollDelta * mZoomSensitivity;

	mOffset *= multiplier;
	mOffsetHeight *= multiplier;
}

Engine::MetaType Engine::TopDownCamControllerComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<TopDownCamControllerComponent>{}, "TopDownCamControllerComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mOffset, "mOffset").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mOffsetHeight, "mOffsetHeight").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mAngle, "mAngle").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mCursorOffsetFactor, "mCursorOffsetFactor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mTarget, "mTarget").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<TopDownCamControllerComponent>(type);
	return type;
}
