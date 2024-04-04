#pragma once
#ifdef EDITOR
#include "Assets/Core/ImportedAsset.h"
#include "Meta/MetaReflect.h"

namespace CE
{
	class Importer
	{
	public:
		virtual ~Importer() = default;

		using ImportResult = std::optional<std::vector<ImportedAsset>>;

		virtual ImportResult Import(const std::filesystem::path& path) const = 0;

		virtual std::vector<std::filesystem::path> CanImportExtensions() const = 0;

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(Importer);
	};
}
#endif // EDITOR
