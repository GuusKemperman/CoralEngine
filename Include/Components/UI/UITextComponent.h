#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetHandle.h"
#include "BasicDataTypes/Colors/LinearColor.h"

namespace CE
{
	class Font;

	class UITextComponent
	{
	public:
		std::string mText = "Sample Text";
		LinearColor mColor{ 1.0f };
		float mSpacing = 0.0f;
		AssetHandle<Font> mFont{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(UITextComponent);
	};
}
