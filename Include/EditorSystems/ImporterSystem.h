#ifdef EDITOR
#include "Core/AssetManager.h"
#include "EditorSystems/EditorSystem.h"

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
		void Import(const std::filesystem::path&) {};

	private:
		static std::vector<std::filesystem::path> GetAllFilesToImport();
		static std::vector<std::filesystem::path> GetAllFilesToImport(const std::filesystem::path& directory);

		static bool WasImportedFrom(const WeakAsset<>& asset, const std::filesystem::path& file);
		static std::pair<TypeId, const Importer*> TryGetImporterForExtension(const std::filesystem::path& extension);

		// Importers are created using the runtime reflection system,
		// which uses placement new for the constructing of objects.
		// Hence, the custom deleter
		static inline std::vector<std::pair<TypeId, std::unique_ptr<Importer, InPlaceDeleter<Importer, true>>>> mImporters;

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ImporterSystem);
	};
}
#endif // EDITOR