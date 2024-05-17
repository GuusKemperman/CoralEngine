#pragma once

namespace cereal
{
	class BinaryInputArchive;
}

namespace CE
{
	class MetaType;

	class AssetFileMetaData
	{
	public:
		// For assets that were imported from somewhere, useful for when we are reimporting
		struct ImporterInfo
		{
			std::filesystem::path mImportedFile{};
			uint32 mImporterVersion{};
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

		static std::optional<AssetFileMetaData> ReadMetaData(std::istream& fromStream);
		static std::optional<AssetFileMetaData> ReadMetaData(const std::filesystem::path& fromFile);

		void WriteMetaData(std::ostream& toStream) const;

	private:
		friend class AssetManager;
		friend class AssetLoadInfo;
		friend class AssetSaveInfo;

		// Backwards compatibility
		static std::optional<AssetFileMetaData> ReadMetaDataV1V2V3(std::istream& fromStream, uint32 version);
		static std::optional<AssetFileMetaData> ReadMetaDataV4(cereal::BinaryInputArchive& fromArchive);

		static constexpr uint32 sMetaDataVersion = 4;
		uint32 mMetaDataVersion{};
		uint32 mAssetVersion{};
		std::string mAssetName{};

		std::reference_wrapper<const MetaType> mClass;

		std::optional<ImporterInfo> mImporterInfo{};
	};
}
