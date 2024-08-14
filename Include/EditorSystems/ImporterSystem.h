#ifdef EDITOR
#include "EditorSystems/EditorSystem.h"

#include <future>

#include "Core/AssetManager.h"
#include "Assets/Importers/Importer.h"
#include "Utilities/Time.h"

namespace CE
{
	class ImporterSystem final :
		public EditorSystem
	{
	public:
		ImporterSystem();
		~ImporterSystem() override;

		void Tick(float deltaTime) override;

		/*
		Imports the specified path, for example ruins.gltf.
		The GLTFImporter then returns all the assets inside
		ruins.gltf, each of which is then stored in their own
		.asset file.

		Note that the assets may not immediately be available;
		importing could mean swapping out existing assets, and
		since it's saver to do this when all assets are unreferenced.
		this is done at the end of the frame.
		*/
		void Import(const std::filesystem::path& fileToImport, std::string_view reasonForImporting);

	private:
		struct ImportRequest
		{
			std::filesystem::path mFile{};
			std::string mReasonForImporting{};
		};

		struct ImportPreview
		{
			ImportRequest mImportRequest{};
			ImportedAsset mImportedAsset;
			std::string mDesiredName{};
		};

		struct ImportFuture
		{
			ImportRequest mImportRequest{};
			std::future<std::optional<std::vector<ImportPreview>>> mImportResult{};
		};

		struct DirToWatch
		{
			std::filesystem::path mDirectory{};
		};

		std::vector<ImportRequest> GetAllFilesToImport();

		std::vector<ImportRequest> GetFilesToImportInDirectory(DirToWatch& directory);

		static bool WasImportedFrom(const WeakAssetHandle<>& asset, const std::filesystem::path& file);

		std::pair<TypeId, std::shared_ptr<const Importer>> TryGetImporterForExtension(const std::filesystem::path& extension);

		bool WouldAssetBeDeletedOrReplacedOnImporting(const WeakAssetHandle<>& asset) const;

		static std::filesystem::path GetPathToSaveAssetTo(const ImportPreview& preview);

		static std::vector<std::filesystem::path> GetDuplicatesOfExistingAssets(const ImportPreview& preview);

		static bool AreDuplicates(const ImportPreview& preview1, const ImportPreview& preview2);

		static std::vector<std::filesystem::path> GetDuplicatesInOtherPreviews(const ImportPreview& preview, std::vector<ImportPreview>::const_iterator begin, std::vector<ImportPreview>::const_iterator end);

		static std::vector<std::filesystem::path> GetDuplicates(const ImportPreview& preview, std::vector<ImportPreview>::const_iterator begin, std::vector<ImportPreview>::const_iterator end);

		static bool WouldAssetBeDeletedOrReplacedOnImporting(const WeakAssetHandle<>& asset, const std::vector<ImportPreview>& toImport);

		void RetrieveImportResultsFromFutures();

		void Preview();

		uint32 ShowDuplicateAssetsErrors();

		uint32 ShowErrorsToWarnAboutDiscardChanges();

		uint32 ShowReadOnlyErrors();

		static void FinishImporting(std::vector<ImportPreview> toImport);

		static bool OpenErrorTab(bool& isOpenAlready, bool& shouldDisplay, std::string_view name);
		static void CloseErrorTab(bool shouldDisplay);

		std::shared_ptr<bool> mWasImportingCancelled{};
		std::vector<std::pair<TypeId, std::shared_ptr<Importer>>> mImporters{};

		// Look up an importer using a file's extension. The value is the index in mImporters
		std::unordered_map<std::filesystem::path, uint32> mImporterLookup{};

		// Look up all the asset that were imported from a certain filename
		std::unordered_multimap<std::filesystem::path, WeakAssetHandle<>> mImportedFromLookUp{};

		std::vector<ImportFuture> mImportFutures{};
		std::vector<ImportPreview> mImportPreview{};
		std::vector<ImportRequest> mFailedFiles{};

		std::array<DirToWatch, 2> mDirectoriesToWatch{};
		Cooldown mCheckDirectoryCooldown{ 10.0f, 10.0f };
		std::future<std::vector<ImportRequest>> mChangedFilesInDirectoriesToWatch{};

		static inline bool sExcludeDuplicates{};
		static inline bool sIgnoreReadOnly = true;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ImporterSystem);
	};
}
#endif // EDITOR