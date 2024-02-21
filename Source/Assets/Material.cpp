#include "Precomp.h"
#include "Assets/Material.h"

#include "Core/AssetManager.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Assets/Texture.h"
#include "Utilities/Reflect/ReflectAssetType.h"

Engine::Material::Material(std::string_view name) :
	Asset(name, MakeTypeId<Material>())
{
}

Engine::Material::Material(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	std::istream& str = loadInfo.GetStream();

	// Everything between start and end is trivially serializable
	char* start = reinterpret_cast<char*>(&mBaseColorFactor);
	const char* end = reinterpret_cast<char*>(&mBaseColorTexture);
	str.read(start, end - start);

	uint16 baseColorTextureNameLength{};
	uint16 metallicRoughnessTextureNameLength{};
	uint16 normalTextureNameLength{};
	uint16 occlusionTextureNameLength{};
	uint16 emissiveTextureNameLength{};

	str.read(reinterpret_cast<char*>(&baseColorTextureNameLength), sizeof(uint16));
	str.read(reinterpret_cast<char*>(&metallicRoughnessTextureNameLength), sizeof(uint16));
	str.read(reinterpret_cast<char*>(&normalTextureNameLength), sizeof(uint16));
	str.read(reinterpret_cast<char*>(&occlusionTextureNameLength), sizeof(uint16));
	str.read(reinterpret_cast<char*>(&emissiveTextureNameLength), sizeof(uint16));

	const auto getTex = [&str, this](const uint16 length) -> std::shared_ptr<const Texture>
		{
			if (length == 0)
			{
				return nullptr;
			}

			std::string textureName(length, '\0');
			str.read(textureName.data(), length);
			std::shared_ptr<const Texture> texture = AssetManager::Get().TryGetAsset<Texture>(Name{ textureName });

			if (texture == nullptr)
			{
				LOG(LogAssets, Warning, "Material {} uses texture {}, but this texture no longer exists",
					GetName(),
					textureName);
			}

			return texture;
		};

	mBaseColorTexture = getTex(baseColorTextureNameLength);
	mMetallicRoughnessTexture = getTex(metallicRoughnessTextureNameLength);
	mNormalTexture = getTex(normalTextureNameLength);
	mOcclusionTexture = getTex(occlusionTextureNameLength);
	mEmissiveTexture = getTex(emissiveTextureNameLength);
}

std::shared_ptr<const Engine::Material> Engine::Material::TryGetDefaultMaterial()
{
	std::shared_ptr<const Material> defaultMat = AssetManager::Get().TryGetAsset<Material>(sDefaultMaterialName);
	
	if (defaultMat == nullptr)
	{
		LOG(LogAssets, Message, "The default material is hardcoded to be {}, but there's no material with that name", sDefaultMaterialName.StringView());
	}
	
	return defaultMat;
}

void Engine::Material::OnSave(AssetSaveInfo& saveInfo) const
{
	OnSave(saveInfo,
		mBaseColorTexture == nullptr ? std::optional<std::string>{} : mBaseColorTexture->GetName(),
		mMetallicRoughnessTexture == nullptr ? std::optional<std::string>{} : mMetallicRoughnessTexture->GetName(),
		mNormalTexture == nullptr ? std::optional<std::string>{} : mNormalTexture->GetName(),
		mOcclusionTexture == nullptr ? std::optional<std::string>{} : mOcclusionTexture->GetName(),
		mEmissiveTexture == nullptr ? std::optional<std::string>{} : mEmissiveTexture->GetName());
}

void Engine::Material::OnSave(AssetSaveInfo& saveInfo, 
	std::optional<std::string> baseColorTextureName, 
	std::optional<std::string> metallicRoughnessTextureName,
	std::optional<std::string> normalTextureName, 
	std::optional<std::string> occlusionTextureName, 
	std::optional<std::string> emissiveTextureName) const
{
	std::ostream& str = saveInfo.GetStream();

	// Everything between start and end is trivially serializable
	const char* start = reinterpret_cast<const char*>(&mBaseColorFactor);
	const char* end = reinterpret_cast<const char*>(&mBaseColorTexture);
	str.write(start, end - start);

	const uint16 baseColorTextureNameLength = static_cast<uint16>(baseColorTextureName.has_value() ? baseColorTextureName->size() : 0);
	const uint16 metallicRoughnessTextureNameLength = static_cast<uint16>(metallicRoughnessTextureName.has_value() ? metallicRoughnessTextureName->size() : 0);
	const uint16 normalTextureNameLength = static_cast<uint16>(normalTextureName.has_value() ? normalTextureName->size() : 0);
	const uint16 occlusionTextureNameLength = static_cast<uint16>(occlusionTextureName.has_value() ? occlusionTextureName->size() : 0);
	const uint16 emissiveTextureNameLength = static_cast<uint16>(emissiveTextureName.has_value() ? emissiveTextureName->size() : 0);

	str.write(reinterpret_cast<const char*>(&baseColorTextureNameLength), sizeof(uint16));
	str.write(reinterpret_cast<const char*>(&metallicRoughnessTextureNameLength), sizeof(uint16));
	str.write(reinterpret_cast<const char*>(&normalTextureNameLength), sizeof(uint16));
	str.write(reinterpret_cast<const char*>(&occlusionTextureNameLength), sizeof(uint16));
	str.write(reinterpret_cast<const char*>(&emissiveTextureNameLength), sizeof(uint16));

	if (baseColorTextureName.has_value()) str.write(baseColorTextureName->c_str(), baseColorTextureName->size());
	if (metallicRoughnessTextureName.has_value()) str.write(metallicRoughnessTextureName->c_str(), metallicRoughnessTextureName->size());
	if (normalTextureName.has_value()) str.write(normalTextureName->c_str(), normalTextureName->size());
	if (occlusionTextureName.has_value()) str.write(occlusionTextureName->c_str(), occlusionTextureName->size());
	if (emissiveTextureName.has_value()) str.write(emissiveTextureName->c_str(), emissiveTextureName->size());
}

Engine::MetaType Engine::Material::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Material>{}, "Material", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{} };

	type.AddField(&Material::mBaseColorFactor, "mBaseColorFactor");
	type.AddField(&Material::mEmissiveFactor, "mEmissiveFactor");
	type.AddField(&Material::mMetallicFactor, "mMetallicFactor");
	type.AddField(&Material::mRoughnessFactor, "mRoughnessFactor");
	type.AddField(&Material::mAlphaCutoff, "mAlphaCutoff");
	type.AddField(&Material::mNormalScale, "mNormalScale");
	type.AddField(&Material::mOcclusionStrength, "mOcclusionStrength");
	type.AddField(&Material::mDoubleSided, "mDoubleSided");
	type.AddField(&Material::mBaseColorTexture, "mBaseColorTexture");
	type.AddField(&Material::mNormalTexture, "mNormalTexture");
	type.AddField(&Material::mOcclusionTexture, "mOcclusionTexture");
	type.AddField(&Material::mMetallicRoughnessTexture, "mMetallicRoughnessTexture");
	type.AddField(&Material::mEmissiveTexture, "mEmissiveTexture");

	ReflectAssetType<Material>(type);
	return type;
}
