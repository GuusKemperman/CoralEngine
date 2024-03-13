#include "Precomp.h"
#include "Assets/Importers/AnimationImporter.h"

#include "Assets/Animation.h"
#include "Components/Animation/Bone.h"
#include "Meta/MetaManager.h"

Engine::ImportedAsset Engine::AnimationImporter::Import(const std::filesystem::path& importedFromFile, 
	const uint32 importerVersion, 
	const Animation& animation)
{
	const MetaType* const animationType = MetaManager::Get().TryGetType<Animation>();
	ASSERT(animationType != nullptr);

	ImportedAsset importedAnimation{ animation.GetName(), *animationType, importedFromFile, importerVersion };

	animation.OnSave(importedAnimation);

	return importedAnimation;
}
