#include "Precomp.h"
#include "Assets/Importers/MaterialImporter.h"

#include "Assets/Material.h"
#include "Meta/MetaManager.h"

Engine::ImportedAsset Engine::MaterialImporter::Import(const std::filesystem::path& importedFromFile, 
    const uint32 importerVersion,
    const Material& material)
{
    const MetaType* const materialType = MetaManager::Get().TryGetType<Material>();
    ASSERT(materialType != nullptr);

    ImportedAsset importedMaterial{ material.GetName(), *materialType, importedFromFile, importerVersion };

    material.OnSave(importedMaterial);

    return importedMaterial;
}
