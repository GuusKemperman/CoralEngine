#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	class Texture;

	class UISpriteComponent
	{
	public:
		glm::vec4 mColor{ 1.0f };
		std::shared_ptr<const Texture> mTexture{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UISpriteComponent);
	};
}
