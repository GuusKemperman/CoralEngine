#include "Precomp.h"
#include "Components/Abilities/ActiveAbilityComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::ActiveAbilityComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<ActiveAbilityComponent>{}, "ActiveAbilityComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&ActiveAbilityComponent::mCastByCharacter, "mCastByCharacter").GetProperties().Add(Props::sIsScriptableTag);
	
	ReflectComponentType<ActiveAbilityComponent>(metaType);

	return metaType;
}
