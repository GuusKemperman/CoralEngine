#include "Precomp.h"
#include "Assets/Texture.h"

#include "stb_image/stb_image.h"

#include "Assets/Core/AssetLoadInfo.h"
#include "Core/Renderer.h"
#include "Core/AssetManager.h"
#include "Rendering/FrameBuffer.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"

CE::Texture::Texture(std::string_view name) :
    Asset(name, MakeTypeId<Texture>())
{}

CE::Texture::Texture(std::string_view name, FrameBuffer&& frameBuffer) :
	Asset(name, MakeTypeId<Texture>()),
	mSize(frameBuffer.mSize),
	mImpl(Renderer::Get().CreateTexturePlatformImpl(std::move(frameBuffer)))
{
}

CE::Texture::Texture(std::string_view name, std::span<const std::byte> pixelsRGBA, glm::ivec2 size) :
	Asset(name, MakeTypeId<Texture>()),
	mSize(size),
	mImpl(Renderer::Get().CreateTexturePlatformImpl(pixelsRGBA, size))
{
}

CE::Texture::Texture(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	const std::string data = StringFunctions::StreamToString(loadInfo.GetStream());
	int channels{};

	stbi_set_flip_vertically_on_load(true);

	std::byte* pixels = reinterpret_cast<std::byte*>(
		stbi_load_from_memory(
			reinterpret_cast<const stbi_uc*>(data.data()), 
			static_cast<int>(data.size()), 
			&mSize.x, &mSize.y, &channels, 4));

	mImpl = Renderer::Get().CreateTexturePlatformImpl({ pixels, static_cast<size_t>(mSize.x * mSize.y * 4) }, mSize);
	stbi_image_free(pixels);
}

CE::MetaType CE::Texture::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Texture>{}, "Texture", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
	ReflectAssetType<Texture>(type);
	return type;
}

CE::AssetHandle<CE::Texture> CE::Texture::TryGetDefaultTexture()
{
	static constexpr std::string_view defaultTextureName = "T_White";

	AssetHandle defaultTexture = AssetManager::Get().TryGetAsset<Texture>(defaultTextureName);

	if (defaultTexture == nullptr)
	{
		LOG(LogAssets, Error, "No default texture {}", defaultTextureName);
	}

	return defaultTexture;
}