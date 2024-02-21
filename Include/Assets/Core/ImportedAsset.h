#pragma once
#include "AssetSaveInfo.h"

namespace Engine
{
	class ImportedAsset :
		public AssetSaveInfo
	{
	public:
		ImportedAsset(const std::string& name, const MetaType& assetClass,
			const std::filesystem::path& importedFromFile, uint32 importerVersion) :
			AssetSaveInfo(name, assetClass, importedFromFile, importerVersion) {}
	};
}