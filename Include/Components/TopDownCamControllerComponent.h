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

		float mOffsetHeight = 3.0f;
		float mOffset = 5.0f;
		float mAngle{};
		float mCursorOffsetFactor = 0.001f;

		float mZoomSensitivity = 0.10f;

		entt::entity mTarget{entt::null};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TopDownCamControllerComponent);
	};
}