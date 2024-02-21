#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class TransformComponent;

	class TopDownCamControllerComponent
	{
	public:
		void ApplyTranslation(TransformComponent& transform, const glm::vec2& cursorDistanceFromScreenCenter) const;
		void UpdateRotation(TransformComponent& transform, const glm::vec3& target);

		float mOffsetHeight = 3.0f;
		float mOffset = 5.0f;
		float mAngle{};
		float mCursorOffsetFactor = 0.01f;

		entt::entity mTarget{entt::null};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TopDownCamControllerComponent);
	};
}