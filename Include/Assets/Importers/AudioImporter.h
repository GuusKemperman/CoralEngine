#pragma once
#ifdef EDITOR
#include "Assets/Importers/Importer.h"

namespace CE
{
	class AudioImporter :
		public Importer
	{
	public:
		ImportResult Import(const std::filesystem::path& path) const override;

		std::vector<std::filesystem::path> CanImportExtensions() const override
		{
			return {
				".wav", ".mp3", ".mp2", ".ogg"
			};
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AudioImporter);
	};
}
#endif // EDITOR