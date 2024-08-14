#pragma once
#include <sstream>

#include "AssetMetaData.h"

namespace CE
{
	class MetaType;

	class AssetSaveInfo
	{
	public:
		AssetSaveInfo(const std::string& name, const MetaType& assetClass, const std::optional<AssetMetaData::ImporterInfo>& importerInfo = std::nullopt);
		
		AssetSaveInfo(AssetSaveInfo&& other) noexcept = default;
		AssetSaveInfo(const AssetSaveInfo&) = delete;

		AssetSaveInfo& operator=(AssetSaveInfo&& other) noexcept = default;
		AssetSaveInfo& operator=(const AssetSaveInfo&) = delete;

		// Returns true on success
		bool SaveToFile(const std::filesystem::path& path) const;
		
		bool IsEmpty() const;

		std::ostream& GetStream() { return mStream; }
		const std::ostream& GetStream() const { return mStream; }

		std::string ToString() const;

		const AssetMetaData& GetMetaData() const { return mMetaData; }

	private:
		// First save to a string stream; if anything goes wrong, our original file is left unmodified
		std::ostringstream mStream{};

		AssetMetaData mMetaData;
	};
}
