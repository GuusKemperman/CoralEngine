#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;

	class TopDownCamControllerComponent
	{
	public:
		void ApplyTranslation(TransformComponent& transform, const glm::vec3& targe, const glm::vec2& cursorDistanceFromScreenCentert) const;
		void UpdateRotation(TransformComponent& transform, const glm::vec3& target, const glm::vec2& cursorDistanceFromScreenCenter);

		void AdjustZoom(const float scrollDelta);
		void RotateCameraAroundTarget(const float angle);

		float mOffsetHeight = 3.0f;
		float mMinOffsetHeight = 1.0f;
		float mMaxOffsetHeight = 100.0f;
		float mOffset = 5.0f;
		float mMinOffset = 1.0f;
		float mMaxOffset = 30.0f;
		float mAngle = 90.0f;
		float mCursorOffsetFactor = 5.0f;

		float mZoomSensitivity = 3.0f;
		float mRotateSensitivity = 100.0f;

		entt::entity mTarget{entt::null};
		glm::vec3 mTargetLocation{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TopDownCamControllerComponent);
	};
}