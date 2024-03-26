#include "Precomp.h"
#include "Components/TopDownCamControllerComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::TopDownCamControllerComponent::ApplyTranslation(TransformComponent& transform,  const glm::vec3& target, glm::vec2 CD) const
{
	const float sn = sin(glm::radians(mAngle - 90.0f));
	const float cs = cos(glm::radians(mAngle - 90.0f));

	const glm::vec2 CDRotated = glm::vec2( CD.x * cs - CD.y * sn, CD.x * sn + CD.y * cs );

	glm::vec3 totalTranslation = target;
	totalTranslation[Axis::Right] += cos(glm::radians(mAngle)) * mOffset + CDRotated.x * mCursorOffsetFactor;
	totalTranslation[Axis::Up] += mOffsetHeight;
	totalTranslation[Axis::Forward] += sin(glm::radians(mAngle)) * mOffset + CDRotated.y * mCursorOffsetFactor;

	transform.SetWorldPosition(totalTranslation);
}

void Engine::TopDownCamControllerComponent::UpdateRotation(TransformComponent& transform, const glm::vec3& target, glm::vec2 CD)
{
	mTargetLocation = target;

	const float sn = sin(glm::radians(mAngle - 90.0f));
	const float cs = cos(glm::radians(mAngle - 90.0f));

	const glm::vec2 CDRotated = glm::vec2( CD.x * cs - CD.y * sn, CD.x * sn + CD.y * cs );

	mTargetLocation[Axis::Right] += CDRotated.x * mCursorOffsetFactor;
	mTargetLocation[Axis::Forward] += CDRotated.y * mCursorOffsetFactor;

	const glm::vec3 direction = glm::normalize(mTargetLocation - transform.GetWorldPosition());

	transform.SetWorldOrientation(glm::quatLookAtLH(direction, sUp));
}

void Engine::TopDownCamControllerComponent::AdjustZoom(const float scrollDelta)
{
	float multiplier = 1.0f + scrollDelta * mZoomSensitivity;

	mOffset = glm::clamp(mOffset * multiplier, mMinOffset, mMaxOffset);
	mOffsetHeight = glm::clamp(mOffsetHeight * multiplier, mMinOffsetHeight, mMaxOffsetHeight);
}

void Engine::TopDownCamControllerComponent::RotateCameraAroundTarget(const float angle)
{
	mAngle = fmod(mAngle + angle * mRotateSensitivity, 360.0f);
	if (mAngle < 0.0f)
	{
		mAngle += 360.0f;
	}
}

Engine::MetaType Engine::TopDownCamControllerComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<TopDownCamControllerComponent>{}, "TopDownCamControllerComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mOffset, "mOffset").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mMinOffset, "mMinOffset").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mMaxOffset, "mMaxOffset").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mOffsetHeight, "mOffsetHeight").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mMinOffsetHeight, "mMinOffsetHeight").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mMaxOffsetHeight, "mMaxOffsetHeight").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mAngle, "mAngle").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mCursorOffsetFactor, "mCursorOffsetFactor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mZoomSensitivity, "mZoomSensitivity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mRotateSensitivity, "mRotateSensitivity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mTarget, "mTarget").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<TopDownCamControllerComponent>(type);
	return type;
}
