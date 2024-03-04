#pragma once

#include "Meta/MetaReflect.h"

namespace Engine
{
	class PointLightComponent
	{
	public:
		glm::vec3 mColor = { 1.0f, 1.0f, 1.0f };
		float mIntensity = 1.0f;
		float mRange = 5.0f;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(PointLightComponent);
	};
}
