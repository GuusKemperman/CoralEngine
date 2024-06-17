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
		struct QuadInfo
		{
			glm::vec4 vertexPositions[4]{};
			glm::vec2 textureCoordinates[4]{};
		};

		/// <summary>
		/// Gives QuadInfo back prepared for rendering for the certain character index given.
		/// </summary>
		void GetCharacterQuadInfo(uint32_t characterIndex, QuadInfo& quadInfo) const;

		/// <summary>
		/// Returns how much space the given character needs to have before the next character appears.
		/// </summary>
		float GetCharacterKerning(uint32_t characterIndex) const;

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
