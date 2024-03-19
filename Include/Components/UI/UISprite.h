#pragma once
#include "Meta/MetaReflect.h"
#include "BasicDataTypes/Colors/LinearColor.h"

namespace Engine
{
	class Texture;

	class UISprite
	{
	public:
		LinearColor mColor{ 1.0f };
		std::shared_ptr<const Texture> mTexture{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UISprite);
	};
}
