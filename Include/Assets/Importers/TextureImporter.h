#pragma once
#include "Assets/Importers/Importer.h"

namespace Engine
{
	class TextureImporter :
		public Importer
	{
	public:
#ifdef EDITOR
		std::optional<std::vector<ImportedAsset>> Import(const std::filesystem::path& path) const override;

		// Will assume one channel -> one byte
		// Expected RGBA
		static std::optional<ImportedAsset> ImportFromMemory(const std::filesystem::path& importedFromFile,
			const std::string& name,
			uint32 importerVersion,
			Span<const char> buffer,
			uint32 width,
			uint32 height);

		std::vector<std::filesystem::path> CanImportExtensions() const override
		{
			return {
			".jpeg",
			".jpg",
			".png",
			".bmp",
			".psd",
			".tga",
			".pic",
			".ppm",
			".pgm",
			".hdr"
			};
		}
#endif // EDITOR

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(TextureImporter);
	};
}