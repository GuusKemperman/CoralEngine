#include "Precomp.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Assets/Ability/Ability.h"
#include "Assets/Ability/Weapon.h"
#include "Systems/AbilitySystem.h"
#include "World/World.h"
#include "Utilities/Reflect/ReflectAssetType.h"

void CE::AbilityInstance::ResetCooldownAndCharges()
{
	mRequirementCounter = 0.f;
	if (mAbilityAsset != nullptr)
	{
		mChargesCounter = mAbilityAsset->mCharges;
	}
}

bool CE::AbilityInstance::operator==(const AbilityInstance& other) const
{
	return mAbilityAsset == other.mAbilityAsset &&
		mKeyboardKeys == other.mKeyboardKeys &&
		mGamepadButtons == other.mGamepadButtons;
}

bool CE::AbilityInstance::operator!=(const AbilityInstance& other) const
{
	return mAbilityAsset != other.mAbilityAsset ||
		mKeyboardKeys != other.mKeyboardKeys ||
		mGamepadButtons != other.mGamepadButtons;
}

CE::MetaType CE::AbilityInstance::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityInstance>{}, "AbilityInstance" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&AbilityInstance::mAbilityAsset, "mAbilityAsset").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstance::mRequirementCounter, "mRequirementCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstance::mChargesCounter, "mChargesCounter").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&AbilityInstance::mKeyboardKeys, "mKeyboardKeys").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstance::mGamepadButtons, "mGamepadButtons").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc([](AbilityInstance& ability, entt::entity castBy, CharacterComponent& characterData)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return AbilitySystem::ActivateAbility(*world, castBy, characterData, ability);

		}, "ActivateAbility", MetaFunc::ExplicitParams<AbilityInstance&, entt::entity, CharacterComponent&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](const AbilityInstance& ability, const CharacterComponent& characterData)
		{
			return AbilitySystem::CanAbilityBeActivated(characterData, ability);

		}, "CanAbilityBeActivated", MetaFunc::ExplicitParams<const AbilityInstance&, const CharacterComponent&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	metaType.AddFunc(&AbilityInstance::ResetCooldownAndCharges, "ResetCooldownAndCharges").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);

	ReflectFieldType<AbilityInstance>(metaType);
	return metaType;
}

void CE::WeaponInstance::ResetCooldownAndAmmo()
{
	mReloadCounter = 0.f;
	if (mWeaponAsset != nullptr)
	{
		mAmmoCounter = mWeaponAsset->mCharges;
	}
}

bool CE::WeaponInstance::operator==(const WeaponInstance& other) const
{
	return mWeaponAsset == other.mWeaponAsset &&
		mKeyboardKeys == other.mKeyboardKeys &&
		mGamepadButtons == other.mGamepadButtons;
}

bool CE::WeaponInstance::operator!=(const WeaponInstance& other) const
{
	return mWeaponAsset != other.mWeaponAsset ||
		mKeyboardKeys != other.mKeyboardKeys ||
		mGamepadButtons != other.mGamepadButtons;
}

CE::MetaType CE::WeaponInstance::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<WeaponInstance>{}, "WeaponInstance"};
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&WeaponInstance::mTimeBetweenShotsCounter, "mTimeBetweenShotsCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mAmmoConsumption, "mAmmoConsumption").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mWeaponAsset, "mWeaponAsset").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mReloadCounter, "mReloadCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mAmmoCounter, "mAmmoCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mKeyboardKeys, "mKeyboardKeys").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mGamepadButtons, "mGamepadButtons").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc([](WeaponInstance& weapon, entt::entity castBy, CharacterComponent& characterData)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return AbilitySystem::ActivateWeapon(*world, castBy, characterData, weapon);

		}, "ActivateWeapon", MetaFunc::ExplicitParams<WeaponInstance&, entt::entity, CharacterComponent&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	metaType.AddFunc([](const WeaponInstance& weapon, const CharacterComponent& characterData)
		{
			return AbilitySystem::CanWeaponBeActivated(characterData, weapon);

		}, "CanWeaponBeActivated", MetaFunc::ExplicitParams<const WeaponInstance&, const CharacterComponent&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	metaType.AddFunc(&WeaponInstance::ResetCooldownAndAmmo, "ResetCooldownAndAmmo").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);

	ReflectFieldType<WeaponInstance>(metaType);
	return metaType;
}
