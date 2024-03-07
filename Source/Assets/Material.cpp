#include "Precomp.h"
#include "Assets/Material.h"

#include "Core/AssetManager.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Assets/Texture.h"
#include "Utilities/ClassVersion.h"
#include "Utilities/Reflect/ReflectAssetType.h"

Engine::Material::Material(std::string_view name) :
	Asset(name, MakeTypeId<Material>())
{
}

Engine::Material::Material(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	switch (loadInfo.GetVersion())
	{
	case 0: LoadV0(loadInfo); break;
	case 1:
	case 2: LoadV1(loadInfo); break;
	default: LOG(LogAssets, Error, "Invalid version {} for material {}", loadInfo.GetVersion(), GetName());
	}
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
	BinaryGSONObject obj{};

	obj.AddGSONMember("BaseColorFactor") << mBaseColorFactor;
	obj.AddGSONMember("EmissiveFactor") << mEmissiveFactor;
	obj.AddGSONMember("MetallicFactor") << mMetallicFactor;
	obj.AddGSONMember("RoughnessFactor") << mRoughnessFactor;
	obj.AddGSONMember("AlphaCutoff") << mAlphaCutoff;
	obj.AddGSONMember("NormalScale") << mNormalScale;
	obj.AddGSONMember("OcclusionStrength") << mOcclusionStrength;
	obj.AddGSONMember("DoubleSided") << mDoubleSided;
	obj.AddGSONMember("BaseColorTexture") << mBaseColorTexture;
	obj.AddGSONMember("NormalTexture") << mNormalTexture;
	obj.AddGSONMember("OcclusionTexture") << mOcclusionTexture;
	obj.AddGSONMember("MetallicRoughnessTexture") << mMetallicRoughnessTexture;
	obj.AddGSONMember("EmissiveTexture") << mEmissiveTexture;

	obj.SaveToBinary(saveInfo.GetStream());
}

void Engine::Material::LoadV0(AssetLoadInfo& loadInfo)
{
	std::istream& str = loadInfo.GetStream();

	// Everything between start and end is trivially serializable
	// So this used to be implemented as follows:
	//
	//		char* start = reinterpret_cast<char*>(&mBaseColorFactor);
	//		const char* end = reinterpret_cast<char*>(&mBaseColorTexture);
	//		str.read(start, end - start);
	//
	// This was not platform or compiler agnostic, which is why we
	// changed the format. This function still supports loading the
	// old format regardless of platform.
	static constexpr size_t start = 56;
	static constexpr size_t bufferSize = 56; // coincidentally the same as start
	static constexpr size_t offsetBaseColorFactor = 56 - start;
	static constexpr size_t offsetEmissiveFactor = 72 - start;
	static constexpr size_t offsetMetallicFactor = 84 - start;
	static constexpr size_t offsetRoughnessFactor = 88 - start;
	static constexpr size_t offsetAlphaCutoff = 92 - start;
	static constexpr size_t offsetNormalScale = 96 - start;
	static constexpr size_t offsetOcclusionStrength = 100 - start;
	static constexpr size_t offsetDoubleSided = 104 - start;

	char buffer[bufferSize];
	str.read(buffer, bufferSize);

	mBaseColorFactor = *reinterpret_cast<LinearColor*>(&buffer[offsetBaseColorFactor]);
	mEmissiveFactor = *reinterpret_cast<glm::vec3*>(&buffer[offsetEmissiveFactor]);
	mMetallicFactor = *reinterpret_cast<float*>(&buffer[offsetMetallicFactor]);
	mRoughnessFactor = *reinterpret_cast<float*>(&buffer[offsetRoughnessFactor]);
	mAlphaCutoff = *reinterpret_cast<float*>(&buffer[offsetAlphaCutoff]);
	mNormalScale = *reinterpret_cast<float*>(&buffer[offsetNormalScale]);
	mOcclusionStrength = *reinterpret_cast<float*>(&buffer[offsetOcclusionStrength]);
	mDoubleSided = *reinterpret_cast<bool*>(&buffer[offsetDoubleSided]);

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
			// 'this' is unused if logging is disabled.
			(void)(this);

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

void Engine::Material::LoadV1(AssetLoadInfo& loadInfo)
{
	BinaryGSONObject obj{};
	const bool success = obj.LoadFromBinary(loadInfo.GetStream());

	if (!success)
	{
		LOG(LogAssets, Error, "Could not load material {}, GSON parsing failed.", GetName());
		return;
	}

	const BinaryGSONMember* serializedBaseColorFactor = obj.TryGetGSONMember("BaseColorFactor");
	const BinaryGSONMember* serializedEmissiveFactor = obj.TryGetGSONMember("EmissiveFactor");
	const BinaryGSONMember* serializedMetallicFactor = obj.TryGetGSONMember("MetallicFactor");
	const BinaryGSONMember* serializedRoughnessFactor = obj.TryGetGSONMember("RoughnessFactor");
	const BinaryGSONMember* serializedAlphaCutoff = obj.TryGetGSONMember("AlphaCutoff");
	const BinaryGSONMember* serializedNormalScale = obj.TryGetGSONMember("NormalScale");
	const BinaryGSONMember* serializedOcclusionStrength = obj.TryGetGSONMember("OcclusionStrength");
	const BinaryGSONMember* serializedDoubleSided = obj.TryGetGSONMember("DoubleSided");
	const BinaryGSONMember* serializedBaseColorTexture = obj.TryGetGSONMember("BaseColorTexture");
	const BinaryGSONMember* serializedNormalTexture = obj.TryGetGSONMember("NormalTexture");
	const BinaryGSONMember* serializedOcclusionTexture = obj.TryGetGSONMember("OcclusionTexture");
	const BinaryGSONMember* serializedMetallicRoughnessTexture = obj.TryGetGSONMember("MetallicRoughnessTexture");
	const BinaryGSONMember* serializedEmissiveTexture = obj.TryGetGSONMember("EmissiveTexture");

	if (serializedBaseColorFactor == nullptr
		|| serializedEmissiveFactor == nullptr
		|| serializedMetallicFactor == nullptr
		|| serializedRoughnessFactor == nullptr
		|| serializedAlphaCutoff == nullptr
		|| serializedNormalScale == nullptr
		|| serializedOcclusionStrength == nullptr
		|| serializedDoubleSided == nullptr
		|| serializedBaseColorTexture == nullptr
		|| serializedNormalTexture == nullptr
		|| serializedOcclusionTexture == nullptr
		|| serializedMetallicRoughnessTexture == nullptr
		|| serializedEmissiveTexture == nullptr)
	{
		LOG(LogAssets, Error, "Could not load material {}, as there were missing values.", GetName());
		return;
	}

	*serializedBaseColorFactor >> mBaseColorFactor;
	*serializedEmissiveFactor >> mEmissiveFactor;
	*serializedMetallicFactor >> mMetallicFactor;
	*serializedRoughnessFactor >> mRoughnessFactor;
	*serializedAlphaCutoff >> mAlphaCutoff;
	*serializedNormalScale >> mNormalScale;
	*serializedOcclusionStrength >> mOcclusionStrength;
	*serializedDoubleSided >> mDoubleSided;
	*serializedBaseColorTexture >> mBaseColorTexture;
	*serializedNormalTexture >> mNormalTexture;
	*serializedOcclusionTexture >> mOcclusionTexture;
	*serializedMetallicRoughnessTexture >> mMetallicRoughnessTexture;
	*serializedEmissiveTexture >> mEmissiveTexture;

	// Occlusion strength is 0 at default in V1, that makes the materials being rendered black at default
	if (loadInfo.GetVersion() == 1)
	{
		mOcclusionStrength = 1.0f;
	}
}

Engine::MetaType Engine::Material::Reflect()
{
	MetaType type = MetaType{ MetaType::T<Material>{}, "Material", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };

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

	SetClassVersion(type, 2);

	ReflectAssetType<Material>(type);
	return type;
}
