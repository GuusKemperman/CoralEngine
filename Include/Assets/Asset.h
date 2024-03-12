#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetFileMetaData.h"

namespace cereal
{
	class BinaryOutputArchive;
	class BinaryInputArchive;
}

namespace Engine
{
	class AssetLoadInfo;
	class AssetSaveInfo;

	class MetaType;
	
	class Asset
	{
	public:
		/*
		The typeId is the typeId of the most derived class.

		Example:
			class Texture : public Asset
			{
			public:
				Texture(std::string_view name) :
					Asset(name, MakeTypeId<Texture>()) {}
			}	
		*/
		Asset(std::string_view name, TypeId myTypeId);
		Asset(AssetLoadInfo& loadInfo);
		virtual ~Asset() = default;

		Asset(Asset&&) noexcept = default;
		Asset(const Asset&) = delete;

		Asset& operator=(Asset&&) = delete;
		Asset& operator=(const Asset&) = delete;

		const std::string& GetName() const { return mName; }
		TypeId GetTypeId() const { return mTypeId; }

		/*
		Saves an asset to memory. AssetSaveInfo has
		functionality in place to allow you to save
		your asset to a file.
		
		In order to implement your own implementation
		for saving an asset, override OnSave.
		*/
		[[nodiscard]] AssetSaveInfo Save(const std::optional<AssetFileMetaData::ImporterInfo>& importerInfo = std::nullopt) const;

	private:
		//********************************//
		//		Virtual functions		  //
		//********************************//

		// Does nothing by default
		virtual void OnSave(AssetSaveInfo& saveInfo) const;

		std::string mName{};
		TypeId mTypeId{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Asset);
	};

	template<typename T>
	static constexpr bool sIsAssetType = std::is_base_of_v<Asset, T>
		&& std::is_constructible_v<Asset, AssetLoadInfo&>;

#ifdef EDITOR
	void InspectAsset(const std::string& name, std::shared_ptr<const Asset>& asset, TypeId assetClass);
#endif // EDITOR

	void SaveAssetReference(cereal::BinaryOutputArchive& ar, const std::shared_ptr<const Asset>& asset);
	void LoadAssetReference(cereal::BinaryInputArchive& ar, std::shared_ptr<const Asset>& asset);
}

#ifdef EDITOR
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#pragma warning(disable : 4201)
#endif
#include "imgui/auto.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace ImGui
{
	template<typename AssetT>
	struct Auto_t<std::shared_ptr<const AssetT>, std::enable_if_t<Engine::sIsAssetType<AssetT>>>
	{
		static void Auto(std::shared_ptr<const AssetT>& var, const std::string& name)
		{
			Engine::InspectAsset(name, reinterpret_cast<std::shared_ptr<const Engine::Asset>&>(var), Engine::MakeTypeId<AssetT>());
		}
		static constexpr bool sIsSpecialized = true;
	};
}
#endif // EDITOR

namespace cereal
{
	template<typename AssetT, std::enable_if_t<Engine::sIsAssetType<AssetT>, bool> = true>
	void save(BinaryOutputArchive& ar, const std::shared_ptr<const AssetT>& value)
	{
		Engine::SaveAssetReference(ar, value);
	}

	template<typename AssetT, std::enable_if_t<Engine::sIsAssetType<AssetT>, bool> = true>
	void load(BinaryInputArchive& ar, std::shared_ptr<const AssetT>& out)
	{
		LoadAssetReference(ar, reinterpret_cast<std::shared_ptr<const Engine::Asset>&>(out));
	}
}
