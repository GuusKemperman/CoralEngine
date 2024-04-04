#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class TransformComponent;

	class FlyCamControllerComponent
	{
	public:
		void ApplyTranslation(TransformComponent& transform, const glm::vec3& timeScaledMovementInput) const;
		void ApplyRotation(TransformComponent& transform, const std::array<glm::quat, 2>& timeScaledRotations) const;

		float mMovementSpeed = 10.0f;
		float mRotationSpeed = 1.0f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(FlyCamControllerComponent);
	};
}
