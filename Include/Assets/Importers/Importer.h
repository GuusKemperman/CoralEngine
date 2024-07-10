#pragma once
#include "Assets/Core/ImportedAsset.h"
#include "Meta/MetaReflect.h"

namespace Engine
{
	class Importer
	{
	public:
		virtual ~Importer() = default;

#ifdef EDITOR
		virtual std::optional<std::vector<ImportedAsset>> Import(const std::filesystem::path& path) const = 0;

		virtual std::vector<std::filesystem::path> CanImportExtensions() const = 0;
#endif // EDITOR

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Importer);
	};
}
