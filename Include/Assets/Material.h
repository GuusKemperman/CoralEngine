#pragma once
#include "Assets/Asset.h"

#include "BasicDataTypes/Colors/LinearColor.h"
#include "Core/AssetHandle.h"

namespace CE
{
	class Texture;

	class Material final :
		public Asset
	{
	public:
		Material(std::string_view name);
		Material(AssetLoadInfo& loadInfo);

		Material(Material&&) noexcept = default;
		Material(const Material&) = default;

		Material& operator=(Material&&) = delete;
		Material& operator=(const Material&) = default;

		static AssetHandle<Material> TryGetDefaultMaterial();

		LinearColor mBaseColorFactor{ 1.0f };		
		glm::vec3 mEmissiveFactor{ 1.0f };
		float mMetallicFactor = 1.0f;
		float mRoughnessFactor = 1.0f;
		float mAlphaCutoff = 0.5f;
		float mNormalScale = 1.0f;
		float mOcclusionStrength = 1.0f;
		bool mDoubleSided = false;

		AssetHandle<Texture> mBaseColorTexture{};
		AssetHandle<Texture> mNormalTexture{};
		AssetHandle<Texture> mOcclusionTexture{};
		AssetHandle<Texture> mMetallicRoughnessTexture{};
		AssetHandle<Texture> mEmissiveTexture{};

		static constexpr Name sDefaultMaterialName = "MT_White"_Name;

	private:
		friend class MaterialImporter;

		void OnSave(AssetSaveInfo& saveInfo) const override;

		void LoadV0(AssetLoadInfo& loadInfo);
		void LoadV1V2(AssetLoadInfo& loadInfo);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Material);
	};
}
