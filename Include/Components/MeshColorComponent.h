#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class MeshColorComponent
	{
		public:
			glm::vec3 mColorMultiplier = glm::vec3(1.f);
			glm::vec3 mColorAddition = glm::vec3(0.f);

		private:
			friend ReflectAccess;
			static MetaType Reflect();
			REFLECT_AT_START_UP(MeshColorComponent);
	};
}

