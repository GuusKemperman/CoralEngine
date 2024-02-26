#include "Precomp.h"
#include "Assets/Texture.h"

#include "Utilities/StringFunctions.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Utilities/StringFunctions.h"
#include "Meta/MetaManager.h"
#include "xsr/backends/common/stb_image.h"

Engine::Texture::Texture(std::string_view name) :
    Asset(name, MakeTypeId<Texture>())
{
}

Engine::Texture::Texture(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
    const std::string data = StringFunctions::StreamToString(loadInfo.GetStream());

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(data.data()), static_cast<int>(data.size()), &width, &height, &channels, 4);
    if (pixels == nullptr)
    {
        LOG(LogAssets, Error, "Invalid texture {}", GetName());
        return;
    }

    mTextureHandle = xsr::create_texture(static_cast<int>(width), static_cast<int>(height), pixels);

    if (!mTextureHandle.is_valid())
    {
        LOG(LogAssets, Error, "Invalid texture {}", GetName());
    }

    stbi_image_free(pixels);
}

Engine::Texture::~Texture()
{
	free_texture(mTextureHandle);
}

Engine::Texture::Texture(Texture&& other) noexcept :
	Asset(std::move(other))
{
    mTextureHandle = other.mTextureHandle;
    other.mTextureHandle.id = 0;
}

Engine::MetaType Engine::Texture::Reflect()
{
    MetaType type = MetaType{ MetaType::T<Texture>{}, "Texture", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
    ReflectAssetType<Texture>(type);
    return type;
}