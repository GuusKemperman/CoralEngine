#pragma once
#ifdef EDITOR
#include "Assets/Importers/Importer.h"

namespace Engine
{
	class GLTFImporter :
		public Importer
	{
	public:

		std::optional<std::vector<ImportedAsset>> Import(const std::filesystem::path& path) const override;

		std::vector<std::filesystem::path> CanImportExtensions() const override
		{
			return { };
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(GLTFImporter);
	};
}
#endif // EDITOR
