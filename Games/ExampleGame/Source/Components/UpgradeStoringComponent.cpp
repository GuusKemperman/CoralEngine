#include "Precomp.h"
#include "Components/UpgradeStoringComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"
#include "Assets/Upgrade.h"

CE::MetaType Game::UpgradeStoringComponent::Reflect()
{
	CE::MetaType metaType = CE::MetaType{ CE::MetaType::T<UpgradeStoringComponent>{}, "UpgradeStoringComponent" };
	metaType.GetProperties().Add(CE::Props::sIsScriptableTag);

	metaType.AddField(&UpgradeStoringComponent::mUpgrade, "mUpgrade").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<UpgradeStoringComponent>(metaType);
	return metaType;
}
