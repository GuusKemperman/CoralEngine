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
	mName(loadInfo.GetMetaData().GetName()),
	mTypeId(loadInfo.GetMetaData().GetClass().GetTypeId())
{
}

CE::AssetSaveInfo CE::Asset::Save(const std::optional<AssetFileMetaData::ImporterInfo>& importerInfo) const
{
	LOG(LogAssets, Verbose, "Begin saving of: {}", GetName());

	MetaType& metaType = *MetaManager::Get().TryGetType(mTypeId);

	AssetSaveInfo saveInfo{ mName, metaType, importerInfo };

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
