#pragma once
#include "AssetSaveInfo.h"

namespace CE
{
	class ImportedAsset :
		public AssetSaveInfo
	{
	public:
		ImportedAsset(const std::string& name, const MetaType& assetClass, const std::filesystem::path& importedFromFile, uint32 importerVersion) :
			AssetSaveInfo(name, assetClass, AssetFileMetaData::ImporterInfo{ importedFromFile, importerVersion, std::filesystem::last_write_time(importedFromFile) })
		{
		}
	};
}