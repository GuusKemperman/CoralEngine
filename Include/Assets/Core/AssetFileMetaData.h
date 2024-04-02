#pragma once

namespace CE
{
	class MetaType;

	class AssetFileMetaData
	{
		friend class AssetManager;
		friend class AssetSaveInfo;
		friend class AssetLoadInfo;

	public:
		// For assets that were imported from somewhere, useful for when we are reimporting
		struct ImporterInfo
		{
			std::filesystem::path mImportedFile{};
			uint32 mImporterVersion{};
			std::filesystem::file_time_type::clock::time_point mImportedFromFileWriteTimeAtTimeOfImporting{};
			bool mWereEditsMadeAfterImporting{};
		};

		// If no version is provided, the current version is used.
		AssetFileMetaData(std::string_view name, const MetaType& assetClass, uint32 assetVersion = std::numeric_limits<uint32>::max(), const std::optional<ImporterInfo>& importerInfo = std::nullopt);

		AssetFileMetaData(const AssetFileMetaData&) = default;
		AssetFileMetaData(AssetFileMetaData&&) noexcept = default;

		~AssetFileMetaData() = default;
	
		AssetFileMetaData& operator=(const AssetFileMetaData&) = default;
		AssetFileMetaData& operator=(AssetFileMetaData&&) noexcept = default;

		const std::string& GetName() const { return mAssetName; }
		const MetaType& GetClass() const { return mClass; }
		uint32 GetAssetVersion() const { return mAssetVersion; }
		uint32 GetMetaDataVersion() const { return mMetaDataVersion; }

		static constexpr uint32 GetCurrentMetaDataVersion() { return sMetaDataVersion; }

		const std::optional<ImporterInfo>& GetImporterInfo() const { return mImporterInfo; }

	private:
		static std::optional<AssetFileMetaData> ReadMetaData(std::istream& fromStream);
		void WriteMetaData(std::ostream& toStream) const;

		// Backwards compatibility
		static std::optional<AssetFileMetaData> ReadMetaDataV0(std::istream& fromStream, uint32 version);
		static std::optional<AssetFileMetaData> ReadMetaDataV1V2(std::istream& fromStream, uint32 version);

		static constexpr uint32 sMetaDataVersion = 2;
		uint32 mMetaDataVersion{};
		uint32 mAssetVersion{};
		std::string mAssetName{};

		std::reference_wrapper<const MetaType> mClass;

		std::optional<ImporterInfo> mImporterInfo{};
	};
}