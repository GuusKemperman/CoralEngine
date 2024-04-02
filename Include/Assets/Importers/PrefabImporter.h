#ifdef EDITOR
#pragma once
#include "Assets/Core/ImportedAsset.h"

namespace CE
{
	class World;

	class PrefabImporter
	{
	public:
		[[nodiscard]] static ImportedAsset MakePrefabFromEntity(const std::filesystem::path& importedFromFile,
			const std::string& name,
			uint32 importerVersion,
			World& world,
			entt::entity entity);
	};
}
#endif // EDITOR
