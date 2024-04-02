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
		void Import(const std::filesystem::path& fileToImport);

	private:
		struct ImportFuture
		{
			std::filesystem::path mFile{};
			std::shared_future<std::optional<std::vector<AssetLoadInfo>>> mImportResult{};
		};

		struct DirToWatch
		{
			std::filesystem::path mDirectory{};
			std::filesystem::file_time_type mDirWriteTimeWhenLastChecked{};
		};

		void ImportAllOutOfDateFiles();
		static std::vector<std::filesystem::path> GetAllFilesToImport(DirToWatch& directory);

		static bool WasImportedFrom(const WeakAsset<>& asset, const std::filesystem::path& file);
		static std::pair<TypeId, std::shared_ptr<const Importer>> TryGetImporterForExtension(const std::filesystem::path& extension);

		static inline std::vector<std::pair<TypeId, std::shared_ptr<Importer>>> mImporters;


		std::vector<ImportFuture> mImportFutures{};


		std::array<DirToWatch, 2> mDirectoriesToWatch{};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ImporterSystem);
	};
}
#endif // EDITOR