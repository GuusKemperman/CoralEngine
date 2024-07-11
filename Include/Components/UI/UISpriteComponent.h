#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Texture;

	class UISpriteComponent
	{
	public:
		glm::vec4 mColor{ 1.0f };

		AssetHandle<Texture> mTexture{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UISpriteComponent);
	};
}
