#include "Precomp.h"
#include "Components/TopDownCamControllerComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::TopDownCamControllerComponent::ApplyTranslation(TransformComponent& transform,  const glm::vec3& target, glm::vec2 CD, float dt) const
{
	const float sn = sin(glm::radians(mRotationAngle - 90.0f));
	const float cs = cos(glm::radians(mRotationAngle - 90.0f));

	const glm::vec2 CDRotated = glm::vec2( CD.x * cs - CD.y * sn, CD.x * sn + CD.y * cs );

	glm::vec3 totalTranslation = target;
	totalTranslation[Axis::Right] += cos(glm::radians(mRotationAngle)) * mOffset.y + CDRotated.x * mCursorOffsetFactor;
	totalTranslation[Axis::Up] = transform.GetLocalPosition()[Axis::Up] + mHeightInterpolationFactor * dt * ((totalTranslation[Axis::Up] + mOffset.x + mOffsetHeight) - transform.GetWorldPosition()[Axis::Up]);
	totalTranslation[Axis::Forward] += sin(glm::radians(mRotationAngle)) * mOffset.y + CDRotated.y * mCursorOffsetFactor;

	transform.SetWorldPosition(totalTranslation);
}

void CE::TopDownCamControllerComponent::UpdateRotation(TransformComponent& transform, const glm::vec3& target, glm::vec2 CD, float dt)
{
	float previousHeight = mTargetLocation[Axis::Up];

	mTargetLocation = target;

	const float sn = sin(glm::radians(mRotationAngle - 90.0f));
	const float cs = cos(glm::radians(mRotationAngle - 90.0f));

	const glm::vec2 CDRotated = glm::vec2( CD.x * cs - CD.y * sn, CD.x * sn + CD.y * cs );

	mTargetLocation[Axis::Right] += CDRotated.x * mCursorOffsetFactor;
	mTargetLocation[Axis::Forward] += CDRotated.y * mCursorOffsetFactor;
	mTargetLocation[Axis::Up] = previousHeight + mHeightInterpolationFactor * dt * (target[Axis::Up] + mOffsetHeight - previousHeight);

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
	type.AddField(&TopDownCamControllerComponent::mTarget, "mTarget").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&TopDownCamControllerComponent::mUseArrowKeysToMove, "mUseArrowKeysToMove").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<TopDownCamControllerComponent>(type);
	return type;
}
