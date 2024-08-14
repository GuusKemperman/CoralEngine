#pragma once

namespace cereal
{
	class BinaryInputArchive;
}

namespace CE
{
	class MetaType;

	class AssetMetaData
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
		AssetMetaData(std::string_view name, 
			const MetaType& assetClass, 
			uint32 assetVersion = std::numeric_limits<uint32>::max(), 
			const std::optional<ImporterInfo>& importerInfo = std::nullopt);

		AssetMetaData(const AssetMetaData&) = default;
		AssetMetaData(AssetMetaData&&) noexcept = default;

		~AssetMetaData() = default;
	
		AssetMetaData& operator=(const AssetMetaData&) = default;
		AssetMetaData& operator=(AssetMetaData&&) noexcept = default;

		const std::string& GetName() const { return mAssetName; }
		const MetaType& GetClass() const { return mClass; }
		uint32 GetAssetVersion() const { return mAssetVersion; }
		uint32 GetMetaDataVersion() const { return mMetaDataVersion; }

		static constexpr uint32 GetCurrentMetaDataVersion() { return sMetaDataVersion; }

		const std::optional<ImporterInfo>& GetImporterInfo() const { return mImporterInfo; }

		static std::optional<AssetMetaData> ReadMetaData(std::istream& fromStream);

		void WriteMetaData(std::ostream& toStream) const;

	private:
		friend class AssetManager;
		friend class AssetLoadInfo;
		friend class AssetSaveInfo;

		// Backwards compatibility
		static std::optional<AssetMetaData> ReadMetaDataV1V2V3(std::istream& fromStream, uint32 version);
		static std::optional<AssetMetaData> ReadMetaDataV4(cereal::BinaryInputArchive& fromArchive);

		static constexpr uint32 sMetaDataVersion = 4;
		uint32 mMetaDataVersion{};
		uint32 mAssetVersion{};
		std::string mAssetName{};

		std::reference_wrapper<const MetaType> mClass;

		std::optional<ImporterInfo> mImporterInfo{};
	};
}
