#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;

	class TopDownCamControllerComponent
	{
	public:
		void ApplyTranslation(TransformComponent& transform, const glm::vec3& target, const glm::vec2& cursorDistanceFromScreenCenter) const;
		void UpdateRotation(TransformComponent& transform, const glm::vec3& target);

		void AdjustZoom(const float scrollDelta);
		void RotateCameraAroundTarget(const float angle);

		float mOffsetHeight = 3.0f;
		float mOffset = 5.0f;
		float mAngle{};
		float mCursorOffsetFactor = 0.001f;

		float mZoomSensitivity = 3.0f;
		float mRotateSensitivity = 100.0f;

		entt::entity mTarget{entt::null};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TopDownCamControllerComponent);
	};
}