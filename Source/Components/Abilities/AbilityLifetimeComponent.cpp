#include "Precomp.h"
#include "Components/Abilities/AbilityLifetimeComponent.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::AbilityLifetimeComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityLifetimeComponent>{}, "AbilityLifetimeComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&AbilityLifetimeComponent::mDuration, "mDuration").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityLifetimeComponent::mDurationTimer, "mDurationTimer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);

	ReflectComponentType<AbilityLifetimeComponent>(metaType);

	return metaType;
}
