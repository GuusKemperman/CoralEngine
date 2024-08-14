#pragma once
#include "Meta/MetaReflect.h"
#include "Assets/Core/AssetMetaData.h"

namespace CE
{
	class AssetLoadInfo;
	class AssetSaveInfo;

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
		Asset(const Asset&) = default;

		Asset& operator=(Asset&&) = default;
		Asset& operator=(const Asset&) = default;

		const std::string& GetName() const { return mName; }

		// Note that this does not rename the asset in the asset manager, nor the asset on file.
		void SetName(std::string_view name) { mName = std::string{ name }; }

		TypeId GetTypeId() const { return mTypeId; }

		/*
		Saves an asset to memory. AssetSaveInfo has
		functionality in place to allow you to save
		your asset to a file.
		
		In order to implement your own implementation
		for saving an asset, override OnSave.
		*/
		[[nodiscard]] AssetSaveInfo Save(const std::optional<AssetMetaData::ImporterInfo>& importerInfo = std::nullopt) const;

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
}
