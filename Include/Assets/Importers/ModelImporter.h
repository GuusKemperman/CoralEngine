#pragma once
#ifdef EDITOR
#include "Assets/Importers/Importer.h"

namespace CE
{
	struct BoneInfo;

	class ModelImporter :
		public Importer
	{
	public:

		Importer::ImportResult Import(const std::filesystem::path& path) const override;

		static std::optional<ImportedAsset> ImportFromMemory(const std::filesystem::path& importedFromFile,
			const std::string& name,
			uint32 importerVersion,
			std::span<const glm::vec3> positions,
			std::span<const uint32> indices,
			std::span<const glm::vec3> normals,
			std::span<const glm::vec3> tangents,
			std::span<const glm::vec2> textureCoordinates);

		std::vector<std::filesystem::path> CanImportExtensions() const override
		{
			// We can import anything assimp can, so this list can be significantly expanded
			// Assimp does not support .blend version 3.1.2
			return {
				".obj", ".gltf", ".glb"
			};
		}

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(ModelImporter);
	};
}
#endif // EDITOR