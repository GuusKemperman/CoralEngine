#include "Precomp.h"
#include "Assets/Importers/AnimationImporter.h"

#include "Assets/Animation/Animation.h"
#include "Meta/MetaManager.h"

CE::ImportedAsset CE::AnimationImporter::Import(const std::filesystem::path& importedFromFile, 
	const uint32 importerVersion, 
	const Animation& animation)
{
	const MetaType* const animationType = MetaManager::Get().TryGetType<Animation>();
	ASSERT(animationType != nullptr);

	ImportedAsset importedAnimation{ animation.GetName(), *animationType, importedFromFile, importerVersion };

	animation.OnSave(importedAnimation);

	return importedAnimation;
}
