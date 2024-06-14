#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Font;

	class UITextComponent
	{
	public:
		std::string mText{};
		glm::vec4 mColor{ 1.0f };
		float mSpacing = 0.02f;
		AssetHandle<Font> mFont{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UITextComponent);
	};
}
