#include "Precomp.h"
#include "Components/TopDownCamControllerComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::TopDownCamControllerComponent::ApplyTranslation(TransformComponent& transform,  const glm::vec3& target, glm::vec2 , float dt) const
{
	const glm::vec3 worldPos = transform.GetWorldPosition();
	const glm::vec3 localPos = transform.GetLocalPosition();

	glm::vec3 totalTranslation = target;
	totalTranslation[Axis::Right] = localPos[Axis::Right] + mCameraLag * dt * ((totalTranslation[Axis::Right] + cos(glm::radians(mRotationAngle)) * mOffset.y) - worldPos[Axis::Right]);
	totalTranslation[Axis::Up] = localPos[Axis::Up] + mHeightInterpolationFactor * dt * ((totalTranslation[Axis::Up] + mOffset.x + mOffsetHeight) - worldPos[Axis::Up]);
	totalTranslation[Axis::Forward] = localPos[Axis::Forward] + mCameraLag * dt * ((totalTranslation[Axis::Forward] + sin(glm::radians(mRotationAngle)) * mOffset.y) - worldPos[Axis::Forward]);

	transform.SetWorldPosition(totalTranslation);
}

void CE::TopDownCamControllerComponent::UpdateRotation(TransformComponent& transform, const glm::vec3& target, glm::vec2 , float dt)
{
	const glm::vec3 prevPos = mTargetLocation;

	mTargetLocation = target;

	const glm::vec3 localPos = transform.GetLocalPosition();

	mTargetLocation[Axis::Right] = prevPos[Axis::Right] + mCameraLag * dt * ((target[Axis::Right] + cos(glm::radians(mRotationAngle)) * mOffset.y) - localPos[Axis::Right]);
	mTargetLocation[Axis::Forward] = prevPos[Axis::Forward] + mCameraLag * dt * ((target[Axis::Forward] + sin(glm::radians(mRotationAngle)) * mOffset.y) - localPos[Axis::Forward]);
	mTargetLocation[Axis::Up] = prevPos[Axis::Up] + mHeightInterpolationFactor * dt * (target[Axis::Up] * mOffsetHeight - prevPos[Axis::Up]);

	const glm::vec3 direction = glm::normalize(mTargetLocation - transform.GetWorldPosition());

	transform.SetWorldOrientation(glm::quatLookAtLH(direction, sUp));
}

void CE::TopDownCamControllerComponent::AdjustZoom(const float scrollDelta)
{
	float multiplier = 1.0f + scrollDelta * mZoomSensitivity;

	mZoom = glm::clamp(mZoom * multiplier, mMinZoom, mMaxZoom);

	mOffset.x = cos(glm::radians(mViewAngle - 90.f)) * mZoom;
	mOffset.y = sin(glm::radians(mViewAngle - 90.f)) * mZoom;
}

void CE::TopDownCamControllerComponent::RotateCameraAroundTarget(const float angle)
{
	mRotationAngle = fmod(mRotationAngle + angle * mRotateSensitivity, 360.0f);
	if (mRotationAngle < 0.0f)
	{
		mRotationAngle += 360.0f;
	}
}

CE::MetaType CE::TopDownCamControllerComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<TopDownCamControllerComponent>{}, "TopDownCamControllerComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mOffsetHeight, "mOffsetHeight").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mZoom, "mZoom").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mMinZoom, "mMinZoom").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mMaxZoom, "mMaxZoom").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mRotationAngle, "mRotationAngle").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mViewAngle, "mViewAngle").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mCursorOffsetFactor, "mCursorOffsetFactor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mZoomSensitivity, "mZoomSensitivity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mRotateSensitivity, "mRotateSensitivity").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mHeightInterpolationFactor, "mHeightInterpolationFactor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mCameraLag, "mCameraLag").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mTarget, "mTarget").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mUseArrowKeysToEdit, "mUseArrowKeysToEdit").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<TopDownCamControllerComponent>(type);
	return type;
}
