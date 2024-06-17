#include "Precomp.h"
#include "Components/UI/UITextComponent.h"
#include "Assets/Texture.h"
#include "Assets/Font.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include <stb_image/stb_truetype.h>

void CE::UITextComponent::GetCharacterQuadInfo(uint32_t characterIndex, QuadInfo& quadInfo) const
{
	const int fontCharacterIndex = mText[characterIndex] - mFont->sFontAtlasMinCharacters;

	stbtt_aligned_quad quad{};
	glm::vec2 dummyPosition{};
	stbtt_GetPackedQuad(
		mFont->GetPackedCharacters().data(),
		mFont->GetTexture().GetWidth(),
		mFont->GetTexture().GetHeight(),
		fontCharacterIndex,
		&dummyPosition.x,
		&dummyPosition.y,
		&quad,
		0);

	quad.x0 = quad.x0 / mFont->sFontSize;
	quad.x1 = quad.x1 / mFont->sFontSize;
	quad.y0 = -quad.y0 / mFont->sFontSize;
	quad.y1 = -quad.y1 / mFont->sFontSize;

	quadInfo.vertexPositions[0] = { quad.x0, quad.y0, 0.0f, 1.0f };
	quadInfo.vertexPositions[1] = { quad.x1, quad.y0, 0.0f, 1.0f };
	quadInfo.vertexPositions[2] = { quad.x0, quad.y1, 0.0f, 1.0f };
	quadInfo.vertexPositions[3] = { quad.x1, quad.y1, 0.0f, 1.0f };

	quadInfo.textureCoordinates[0] = { quad.s0, quad.t0 };
	quadInfo.textureCoordinates[1] = { quad.s1, quad.t0 };
	quadInfo.textureCoordinates[2] = { quad.s0, quad.t1 };
	quadInfo.textureCoordinates[3] = { quad.s1, quad.t1 };
}

float CE::UITextComponent::GetCharacterKerning(uint32_t characterIndex) const
{
	const int glyphIndex = stbtt_FindGlyphIndex(&mFont->GetInfo(), mText[characterIndex]);

	int iAdvance = 0, iBearing = 0;
	stbtt_GetGlyphHMetrics(&mFont->GetInfo(), glyphIndex, &iAdvance, &iBearing);
	float advance = iAdvance * mFont->GetScale() / mFont->sFontSize;

	return advance + mSpacing;
}

CE::MetaType CE::UITextComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<UITextComponent>{}, "UITextComponent" };
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);

	type.AddField(&UITextComponent::mText, "mText").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UITextComponent::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UITextComponent::mSpacing, "mSpacing").GetProperties().Add(Props::sIsScriptableTag);
	type.AddField(&UITextComponent::mFont, "mFont").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<UITextComponent>(type);
	return type;
}