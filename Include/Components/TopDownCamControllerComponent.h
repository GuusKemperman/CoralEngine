#pragma once
#include "Meta/MetaReflect.h"

namespace CE
{
	class TransformComponent;

	class TopDownCamControllerComponent
	{
	public:
		void ApplyTranslation(TransformComponent& transform, const glm::vec3& targe, glm::vec2 cursorDistanceFromScreenCentert, float dt) const;
		void UpdateRotation(TransformComponent& transform, const glm::vec3& target, glm::vec2 cursorDistanceFromScreenCenter, float dt);

		void AdjustZoom(const float scrollDelta);
		void RotateCameraAroundTarget(const float angle);

		float mOffsetHeight = 3.0f;
		glm::vec2 mOffset = {5.f, 5.f};
		float mZoom = 1.0f;
		float mMinZoom = 0.5f;
		float mMaxZoom = 10.0f;
		float mRotationAngle = 270.0f;
		float mViewAngle = 45.0f;
		float mCursorOffsetFactor = 0.0f;

		float mZoomSensitivity = 3.0f;
		float mRotateSensitivity = 100.0f;

		float mHeightInterpolationFactor = 0.7f;

		entt::entity mTarget{entt::null};
		glm::vec3 mTargetLocation{};

		bool mUseArrowKeysToMove{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TopDownCamControllerComponent);
	};
}