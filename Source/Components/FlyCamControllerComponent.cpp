#include "Precomp.h"
#include "Components/FlyCamControllerComponent.h"

#include "Components/TransformComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::FlyCamControllerComponent::AdjustMovementSpeed(const bool movementSpeedMultiply)
{
	if (movementSpeedMultiply)
	{
		mMovementSpeed *= mMovementSpeedAdjustmentSensitivity;
	}
	else
	{
		mMovementSpeed /= mMovementSpeedAdjustmentSensitivity;
	}
}

void Engine::FlyCamControllerComponent::ApplyTranslation(TransformComponent& transform, const glm::vec3& timeScaledMovementInput) const
{
	const glm::vec3 scaledMovementInput = timeScaledMovementInput * mMovementSpeed;

	glm::vec3 totalTranslation{};
	for (uint32 i = 0; i < 3; i++)
	{
		totalTranslation += scaledMovementInput[i] * transform.GetWorldAxis(static_cast<Axis>(i));
	}

	transform.TranslateWorldPosition(totalTranslation);
	
}

void Engine::FlyCamControllerComponent::ApplyRotation(TransformComponent& transform, const std::array<glm::quat, 2>& timeScaledRotations) const
{
	glm::quat orientation = transform.GetWorldOrientation();
	orientation = (timeScaledRotations[1] * mRotationSpeed) * orientation * (timeScaledRotations[0] * mRotationSpeed);
	transform.SetWorldOrientation(orientation);
}

Engine::MetaType Engine::FlyCamControllerComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<FlyCamControllerComponent>{}, "FlyCamControllerComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	type.AddField(&FlyCamControllerComponent::mMovementSpeed, "mMovementSpeed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&FlyCamControllerComponent::mRotationSpeed, "mRotationSpeed").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&FlyCamControllerComponent::mMovementSpeedAdjustmentSensitivity, "mMovementSpeedAdjustmentSensitivity").GetProperties().Add(Props::sIsScriptableTag);
	ReflectComponentType<FlyCamControllerComponent>(type);
	return type;
}
