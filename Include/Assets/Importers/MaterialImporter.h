#ifdef EDITOR
#pragma once
#include "Assets/Core/ImportedAsset.h"

namespace CE
{
	class Material;

	class MaterialImporter
	{
	public:
		[[nodiscard]] static ImportedAsset Import(const std::filesystem::path& importedFromFile,
			uint32 importerVersion,
			const Material& material);
	};
}
#endif // EDITOR
