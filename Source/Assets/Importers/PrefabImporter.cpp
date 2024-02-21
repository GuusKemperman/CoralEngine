#include "Precomp.h"
#include "Assets/Importers/PrefabImporter.h"

#include "Assets/Prefabs/Prefab.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

Engine::ImportedAsset Engine::PrefabImporter::MakePrefabFromEntity(const std::filesystem::path& importedFromFile, 
    const std::string& name, 
    const uint32 importerVersion, 
    World& world, 
    entt::entity entity)
{
    const MetaType* const prefabType = MetaManager::Get().TryGetType<Prefab>();
    ASSERT(prefabType != nullptr);

    ImportedAsset prefab{ name, *prefabType, importedFromFile, importerVersion };
    Prefab::OnSave(prefab, name, world, entity);
    return prefab;
}
