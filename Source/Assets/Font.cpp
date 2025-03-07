#include "Precomp.h"
#include "Assets/Font.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Texture.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"

#include "glm/gtx/integer.hpp"
#include "Utilities/Math.h"

CE::Font::Font(std::string_view name) :
    Asset(name, MakeTypeId<Font>())
{
}

CE::Font::Font(AssetLoadInfo& loadInfo) :
    Asset(loadInfo),
    mFileContents(StringFunctions::StreamToString(loadInfo.GetStream()))
{
    const unsigned char* bufferPtr = reinterpret_cast<unsigned char*>(mFileContents.data());
    if (!stbtt_InitFont(&mInfo, bufferPtr, 0))
    {
        LOG(LogAssets, Error, "Initializing font asset failed: ", GetName());
    }

    // Calculate by what factor to scale the font at render time
    int ascent;
    int descent;
    int lineGap;
    stbtt_GetFontVMetrics(&mInfo, &ascent, &descent, &lineGap);
    mScale = sFontSize / (ascent - descent);
    
    // Calculate how big the font atlas image should be
    int requiredPixels = (int)sFontSize * (int)sFontSize * sFontAtlasNumCharacters;
    uint32_t dimension = Math::NextPowerOfTwo(glm::sqrt(requiredPixels));
    uint32_t numPixels = dimension * dimension;

    unsigned char* bitmap = static_cast<unsigned char*>(malloc(numPixels));
    mPackedCharacters.resize(sFontAtlasNumCharacters);

    // Pack font
    stbtt_pack_context packContext{};
    stbtt_PackBegin(&packContext, bitmap, dimension, dimension, 0, 2, nullptr);
    stbtt_PackFontRange(
        &packContext,
        bufferPtr,
        0, sFontSize,
        sFontAtlasMinCharacters,
        sFontAtlasNumCharacters,
        mPackedCharacters.data());
    stbtt_PackEnd(&packContext);

    // Expand bitmap to all color channels
    unsigned char* rgba = new unsigned char[numPixels * 4];
    for (uint32_t i = 0; i < numPixels; ++i)
    {
        rgba[i * 4 + 0] = bitmap[i];
        rgba[i * 4 + 1] = bitmap[i];
        rgba[i * 4 + 2] = bitmap[i];
        rgba[i * 4 + 3] = bitmap[i];
    }

    mTexture = std::make_unique<Texture>(std::string("FontAtlas-") + GetName(), dimension, dimension, rgba);

    delete[] rgba;
    free(bitmap);
}

CE::MetaType CE::Font::Reflect()
{
    MetaType type = MetaType{ MetaType::T<Font>{}, "Font", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
    type.GetProperties().Add(Props::sCannotReferenceOtherAssetsTag);
    ReflectAssetType<Font>(type);
    return type;
}
