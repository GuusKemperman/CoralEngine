#include "Precomp.h"
#include "Assets/Texture.h"

#include "Core/AssetManager.h"
#include "Utilities/Reflect/ReflectAssetType.h"

CE::Texture::Texture(std::string_view name) :
    Asset(name, MakeTypeId<Texture>())
{}

CE::MetaType CE::Texture::Reflect()
{
    MetaType type = MetaType{ MetaType::T<Texture>{}, "Texture", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
	type.GetProperties().Add(Props::sCannotReferenceOtherAssetsTag);
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