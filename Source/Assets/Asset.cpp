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

Engine::Asset::Asset(std::string_view name, TypeId myTypeId) :
	mName(name),
	mTypeId(myTypeId)
{
}

Engine::Asset::Asset(AssetLoadInfo& loadInfo) :
	mName(loadInfo.GetName()),
	mTypeId(loadInfo.GetAssetClass().GetTypeId())
{
}

Engine::AssetSaveInfo Engine::Asset::Save() const
{
	LOG_FMT(LogAssets, Verbose, "Begin saving of: {}", GetName());
	AssetSaveInfo saveInfo{ mName, *MetaManager::Get().TryGetType(mTypeId) };
	OnSave(saveInfo);
	LOG_FMT(LogAssets, Verbose, "Finished saving of: {}", GetName());
	return saveInfo;
}

void Engine::Asset::OnSave(AssetSaveInfo&) const
{
	LOG(LogAssets, Verbose, "OnSave was not overriden for this asset class");
}

Engine::MetaType Engine::Asset::Reflect()
{
	MetaType type = MetaType{MetaType::T<Asset>{}, "Asset", MetaType::Ctor<AssetLoadInfo&>{} };
	return type;
}

#ifdef EDITOR

void Engine::InspectAsset(const std::string& name, std::shared_ptr<const Asset>& asset, const TypeId assetClass)
{
	std::vector<WeakAsset<Asset>> allAssets = AssetManager::Get().GetAllAssets();

	std::optional<WeakAsset<Asset>> selectedAsset = Search::DisplayDropDownWithSearchBar<Asset>(name.c_str(), asset == nullptr ? "None" : asset->GetName().data(),
		[assetClass](const WeakAsset<Asset>& asset)
		{
			return asset.GetAssetClass().IsDerivedFrom(assetClass);
		});

	if (selectedAsset.has_value())
	{
		asset = selectedAsset->MakeShared();
	}

	std::optional<WeakAsset<Asset>> receivedAsset = DragDrop::PeekAsset(assetClass);

	if (receivedAsset.has_value()
		&& DragDrop::AcceptAsset())
	{
		asset = receivedAsset->MakeShared();
	}
}

#endif // EDITOR

void Engine::SaveAssetReference(cereal::BinaryOutputArchive& ar, const std::shared_ptr<const Asset>& asset)
{
	if (asset != nullptr)
	{
		const Name::HashType hash = Name::HashString(asset->GetName());
		ASSERT_LOG_FMT(hash != 0, "Hash of {} was 0, but this value is reserved for null assets. Please rename the asset.", asset->GetName());
		ar(hash);
	}
	else
	{
		ar(Name::HashType{});
	}
}

void Engine::LoadAssetReference(cereal::BinaryInputArchive& ar, std::shared_ptr<const Asset>& asset)
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
		LOG_FMT(LogAssets, Warning, "Asset whose name generated the hash {} no longer exists", assetNameHash);
	}
}
