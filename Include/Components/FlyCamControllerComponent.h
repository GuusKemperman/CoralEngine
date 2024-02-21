#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;

	class FlyCamControllerComponent
	{
	public:
		void AdjustMovementSpeed(bool movementSpeedMultiply);

		void ApplyTranslation(TransformComponent& transform, const glm::vec3& timeScaledMovementInput) const;
		void ApplyRotation(TransformComponent& transform, const std::array<glm::quat, 2>& timeScaledRotations) const;


		float mMovementSpeed = 10.0f;
		float mRotationSpeed = 1.0f;
		float mMovementSpeedAdjustmentSensitivity = 1.15f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(FlyCamControllerComponent);
	};
}
