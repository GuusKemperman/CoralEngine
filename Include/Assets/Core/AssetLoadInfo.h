#pragma once
#include "AssetFileMetaData.h"

namespace CE
{
	class AssetSaveInfo;

	class AssetLoadInfo
	{
		// Use LoadFromFile instead
		AssetLoadInfo(const std::filesystem::path& fromFile);

		// Use LoadFromStream
		AssetLoadInfo(std::unique_ptr<std::istream> stream);

	public:
		AssetLoadInfo(const AssetSaveInfo& saveInfo);

		// As if you've loaded an empty file
		AssetLoadInfo(const MetaType& assetClass, std::string_view name);

		static std::optional<AssetLoadInfo> LoadFromFile(const std::filesystem::path& fromFile);
		static std::optional<AssetLoadInfo> LoadFromStream(std::unique_ptr<std::istream> stream);

		std::istream& GetStream() { return *mStream; }

		const AssetFileMetaData& GetMetaData() const { return *mMetaData; }

	private:
		// Returns true on succes
		bool ConstructFromCurrentStream();

		std::unique_ptr<std::istream> mStream;
		
		friend class AssetManager;
		std::unique_ptr<AssetFileMetaData> mMetaData;
	};
}