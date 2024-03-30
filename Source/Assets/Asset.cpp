#include "Precomp.h"
#include "Assets/Asset.h"

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Utilities/Imgui/ImguiDragDrop.h"
#include "Utilities/Search.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Core/AssetManager.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"

CE::Asset::Asset(std::string_view name, TypeId myTypeId) :
	mName(name),
	mTypeId(myTypeId)
{
}

CE::Asset::Asset(AssetLoadInfo& loadInfo) :
	mName(loadInfo.GetName()),
	mTypeId(loadInfo.GetAssetClass().GetTypeId())
{
}

CE::AssetSaveInfo CE::Asset::Save(const std::optional<AssetFileMetaData::ImporterInfo>& importerInfo) const
{
	LOG(LogAssets, Verbose, "Begin saving of: {}", GetName());

	MetaType& metaType = *MetaManager::Get().TryGetType(mTypeId);

	AssetSaveInfo saveInfo = importerInfo.has_value()
		? AssetSaveInfo{ mName, metaType, importerInfo->mImportedFile, importerInfo->mImporterVersion }
		: AssetSaveInfo{ mName, metaType };

	OnSave(saveInfo);
	LOG(LogAssets, Verbose, "Finished saving of: {}", GetName());
	return saveInfo;
}

void CE::Asset::OnSave(AssetSaveInfo&) const
{
	LOG(LogAssets, Verbose, "OnSave was not overriden for this asset class");
}

CE::MetaType CE::Asset::Reflect()
{
	MetaType type = MetaType{MetaType::T<Asset>{}, "Asset", MetaType::Ctor<AssetLoadInfo&>{} };
	return type;
}

#ifdef EDITOR

struct NullAsset { };

namespace CE::Search
{
	template<>
	struct SearchBarOptionsCollector<NullAsset>
	{
		template<typename... Types, typename FilterType>
		static void AddOptions(Search::Choices<Types...>& insertInto, const FilterType&)
		{
			// The space is intentional, it's a 'hack' to make
			// sure None is always the first option, since
			// the options are sorted in a lexicographical order.
			insertInto.emplace_back(" None", NullAsset{});
		}
	};
}

void CE::InspectAsset(const std::string& name, std::shared_ptr<const Asset>& asset, const TypeId assetClass)
{
	auto selectedAsset = Search::DisplayDropDownWithSearchBar<Asset, NullAsset>(name.c_str(), asset == nullptr ? "None" : asset->GetName().data(),
		[assetClass](const std::variant<WeakAsset<Asset>, NullAsset>& asset)
		{
			return std::get<WeakAsset<Asset>>(asset).GetAssetClass().IsDerivedFrom(assetClass);
		});

	if (selectedAsset.has_value())
	{
		if (std::holds_alternative<NullAsset>(*selectedAsset))
		{
			asset = nullptr;
		}
		else
		{
			asset = std::get<WeakAsset<Asset>>(*selectedAsset).MakeShared();
		}
	}

	std::optional<WeakAsset<Asset>> receivedAsset = DragDrop::PeekAsset(assetClass);

	if (receivedAsset.has_value()
		&& DragDrop::AcceptAsset())
	{
		asset = receivedAsset->MakeShared();
	}
}

#endif // EDITOR

void CE::SaveAssetReference(cereal::BinaryOutputArchive& ar, const std::shared_ptr<const Asset>& asset)
{
	if (asset != nullptr)
	{
		const Name::HashType hash = Name::HashString(asset->GetName());
		ASSERT_LOG(hash != 0, "Hash of {} was 0, but this value is reserved for null assets. Please rename the asset.", asset->GetName());
		ar(hash);
	}
	else
	{
		ar(Name::HashType{});
	}
}

void CE::LoadAssetReference(cereal::BinaryInputArchive& ar, std::shared_ptr<const Asset>& asset)
{
	Name::HashType assetNameHash{};
	ar(assetNameHash);

	if (assetNameHash == 0)
	{
		asset = nullptr;
		return;
	}

	asset = AssetManager::Get().TryGetAsset(Name{ assetNameHash });

	if (asset == nullptr)
	{
		LOG(LogAssets, Warning, "Asset whose name generated the hash {} no longer exists", assetNameHash);
	}
}
