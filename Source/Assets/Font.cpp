#include "Precomp.h"
#include "Assets/Font.h"

#include <glm/gtx/integer.hpp>

#include "Assets/Core/AssetLoadInfo.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Utilities/Math.h"

#include "stb_image/stbi_image_write.h"

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
    int a***REMOVED***nt;
    int de***REMOVED***nt;
    int lineGap;
    stbtt_GetFontVMetrics(&mInfo, &a***REMOVED***nt, &de***REMOVED***nt, &lineGap);
    mScale = (sFontSize / a***REMOVED***nt - de***REMOVED***nt);
    
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

    // Make image monochrome
    unsigned char* rgba = new unsigned char[numPixels * 4];
    for (uint32_t i = 0; i < numPixels; ++i)
    {
        rgba[i * 4 + 0] = bitmap[i];
        rgba[i * 4 + 1] = 0;
        rgba[i * 4 + 2] = 0;
        rgba[i * 4 + 3] = 255;
    }

    std::string filename = std::string("FontAtlas-") + GetName() + ".png";
    stbi_write_png(filename.c_str(), dimension, dimension, 4, rgba, 0);

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
