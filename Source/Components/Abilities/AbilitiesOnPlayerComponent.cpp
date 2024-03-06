#include "Precomp.h"
#include "Components/Abilities/AbilitiesOnPlayerComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"
#include "Assets/Ability.h"

Engine::MetaType Engine::AbilitiesOnPlayerComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilitiesOnPlayerComponent>{}, "AbilitiesOnPlayerComponent" };
	
	metaType.AddField(&AbilitiesOnPlayerComponent::mAbilitiesToInput, "mAbilitiesToInput").GetProperties();

	ReflectComponentType<AbilitiesOnPlayerComponent>(metaType);

	return metaType;
}

bool Engine::AbilityInstanceWithInputs::operator==(const AbilityInstanceWithInputs& other) const
{
	return mAbilityAsset == other.mAbilityAsset;
}

bool Engine::AbilityInstanceWithInputs::operator!=(const AbilityInstanceWithInputs& other) const
{
	return mAbilityAsset != other.mAbilityAsset;
}

void Engine::AbilityInstanceWithInputs::DisplayWidget()
{
	ShowInspectUI("mAbilityAsset", mAbilityAsset);
	ShowInspectUI("mKeyboardKeys", mKeyboardKeys);
	ShowInspectUI("mGamepadButtons", mGamepadButtons);
}

Engine::MetaType Engine::AbilityInstanceWithInputs::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityInstanceWithInputs>{}, "AbilityInstanceWithInputs" };

	metaType.AddField(&AbilityInstanceWithInputs::mAbilityAsset, "mAbilityAsset").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstanceWithInputs::mRequirementCounter, "mRequirementCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstanceWithInputs::mChargesCounter, "mChargesCounter").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&AbilityInstanceWithInputs::mKeyboardKeys, "mKeyboardKeys").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstanceWithInputs::mGamepadButtons, "mGamepadButtons").GetProperties().Add(Props::sIsScriptableTag);

	return metaType;
}
