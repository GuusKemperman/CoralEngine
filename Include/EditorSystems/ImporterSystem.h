#ifdef EDITOR
#include "EditorSystems/EditorSystem.h"

#include <future>

#include "Core/AssetManager.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Importers/Importer.h"
#include "Utilities/MemFunctions.h"

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

		struct ImportFuture
		{
			ImportRequest mImportRequest{};
			std::future<Importer::ImportResult> mImportResult{};
		};

		struct ImportPreview
		{
			ImportRequest mImportRequest{};
			ImportedAsset mImportedAsset;
		};

		struct DirToWatch
		{
			std::filesystem::path mDirectory{};
			std::filesystem::file_time_type mDirWriteTimeWhenLastChecked{};
		};

		void ImportAllOutOfDateFiles();

		std::vector<ImportRequest> GetAllFilesToImport(DirToWatch& directory);

		static bool WasImportedFrom(const WeakAsset<>& asset, const std::filesystem::path& file);

		std::pair<TypeId, std::shared_ptr<const Importer>> TryGetImporterForExtension(const std::filesystem::path& extension);

		bool WouldAssetBeDeletedOrReplacedOnImporting(const WeakAsset<>& asset) const;

		std::filesystem::path GetPathToSaveAssetTo(const ImportPreview& preview) const;

		static std::filesystem::path GetPathToSaveAssetTo(const ImportPreview& preview, const std::vector<ImportPreview>& toImport);

		static bool WouldAssetBeDeletedOrReplacedOnImporting(const WeakAsset<>& asset, const std::vector<ImportPreview>& toImport);

		void RetrieveImportResultsFromFutures();

		void Preview();

		// Check to see if our user submitted multiple assets with the same name
		uint32 ShowDuplicateImportedAssetsErrors();

		uint32 ShowDuplicateAssetsErrors();

		uint32 ShowErrorsToWarnAboutDiscardChanges();

		uint32 ShowReadOnlyErrors();

		static void FinishImporting(std::vector<ImportPreview> toImport);

		static bool OpenErrorTab(bool& isOpenAlready, bool& shouldDisplay, std::string_view name);
		static void CloseErrorTab(bool shouldDisplay);

		std::shared_ptr<bool> mWasImportingCancelled{};
		std::vector<std::pair<TypeId, std::shared_ptr<Importer>>> mImporters{};
		std::vector<ImportFuture> mImportFutures{};
		std::vector<ImportPreview> mImportPreview{};
		std::vector<ImportRequest> mFailedFiles{};

		std::array<DirToWatch, 2> mDirectoriesToWatch{};

		static inline bool sIsOpen;
		bool mHasWindowPoppedUp{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ImporterSystem);
	};
}
#endif // EDITOR