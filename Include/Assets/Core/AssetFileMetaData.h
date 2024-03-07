#pragma once

namespace Engine
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
		};

	private:
		// If no version is provided, the current version is used.
		AssetFileMetaData(std::string_view name, const MetaType& assetClass, uint32 version = (std::numeric_limits<int>::max)(), const std::optional<ImporterInfo>& importerInfo = std::nullopt);

	public:
		AssetFileMetaData(const AssetFileMetaData&) = default;
		AssetFileMetaData(AssetFileMetaData&&) noexcept = default;

		~AssetFileMetaData() = default;
	
		AssetFileMetaData& operator=(const AssetFileMetaData&) = default;
		AssetFileMetaData& operator=(AssetFileMetaData&&) noexcept = default;

		const std::string& GetName() const { return mAssetName; }
		const MetaType& GetClass() const { return mClass; }
		uint32 GetVersion() const { return mAssetVersion; }

		const std::optional<ImporterInfo>& GetImporterInfo() const { return mImporterInfo; }

	private:
		static std::optional<AssetFileMetaData> ReadMetaData(std::istream& fromStream);
		void WriteMetaData(std::ostream& toStream) const;

		// Backwards compatibility
		static std::optional<AssetFileMetaData> ReadMetaDataV0(std::istream& fromStream);
		static std::optional<AssetFileMetaData> ReadMetaDataV1(std::istream& fromStream);

		static constexpr uint32 sMetaDataVersion = 1;
		uint32 mAssetVersion{};
		std::string mAssetName{};

		std::reference_wrapper<const MetaType> mClass;

		std::optional<ImporterInfo> mImporterInfo{};
	};
}