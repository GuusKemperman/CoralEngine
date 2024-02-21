#pragma once
#include "Assets/Asset.h"

#include "BasicDataTypes/Colors/LinearColor.h"

namespace Engine
{
	class Texture;

	class Material final :
		public Asset
	{
	public:
		Material(std::string_view name);
		Material(AssetLoadInfo& loadInfo);

		static std::shared_ptr<const Material> TryGetDefaultMaterial();

		// NOTE: Before adding, removing and reordering variables,
		// take a look at OnSave and OnLoad, as the order matters.

		LinearColor mBaseColorFactor{ 1.0f };		
		glm::vec3 mEmissiveFactor{};
		float mMetallicFactor = 1.0f;
		float mRoughnessFactor = 1.0f;
		float mAlphaCutoff = .5f;
		float mNormalScale = 1.0f;

		// occludedColor = lerp(color, color * <sampled occlusion
		// texture value>, <occlusion strength>
		float mOcclusionStrength{};
		bool mDoubleSided{};

		std::shared_ptr<const Texture> mBaseColorTexture{};
		std::shared_ptr<const Texture> mNormalTexture{};
		std::shared_ptr<const Texture> mOcclusionTexture{};
		std::shared_ptr<const Texture> mMetallicRoughnessTexture{};
		std::shared_ptr<const Texture> mEmissiveTexture{};

		static constexpr Name sDefaultMaterialName = "MT_White"_Name;

	private:
		friend class MaterialImporter;

		void OnSave(AssetSaveInfo& saveInfo) const override;

		void OnSave(AssetSaveInfo& saveInfo,
			std::optional<std::string> baseColorTextureName,
			std::optional<std::string> metallicRoughnessTextureName,
			std::optional<std::string> normalTextureName,
			std::optional<std::string> occlusionTextureName,
			std::optional<std::string> emissiveTextureName) const;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Material);
	};
}
