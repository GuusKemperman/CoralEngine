#include "Precomp.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Meta/ReflectedTypes/STD/ReflectOptional.h"
#include "Assets/Ability/Ability.h"
#include "Assets/Ability/Weapon.h"
#include "Components/PlayerComponent.h"
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

CE::Weapon* CE::WeaponInstance::InitializeRuntimeWeapon()
{
	if (mWeaponAsset == nullptr)
	{
		LOG(LogAbilitySystem, Error, "mWeaponAsset not set.");
		return nullptr;
	}
	mRuntimeWeapon.emplace(*mWeaponAsset.Get());
	return &mRuntimeWeapon.value();
}

bool CE::WeaponInstance::operator==(const WeaponInstance& other) const
{
	return mWeaponAsset == other.mWeaponAsset &&
		mKeyboardKeys == other.mKeyboardKeys &&
		mGamepadButtons == other.mGamepadButtons &&
		mReloadKeyboardKeys == other.mReloadKeyboardKeys &&
		mReloadGamepadButtons == other.mReloadGamepadButtons;
}

bool CE::WeaponInstance::operator!=(const WeaponInstance& other) const
{
	return !(*this == other);
}

CE::MetaType CE::WeaponInstance::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<WeaponInstance>{}, "WeaponInstance"};
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&WeaponInstance::mWeaponAsset, "mWeaponAsset").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mReloadCounter, "mReloadCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mAmmoCounter, "mAmmoCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mShotDelayCounter, "mShotDelayCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mAmmoConsumption, "mAmmoConsumption").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mShotsAccumulated, "mShotsAccumulated").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mKeyboardKeys, "mKeyboardKeys").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mGamepadButtons, "mGamepadButtons").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mReloadKeyboardKeys, "mReloadKeyboardKeys").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mReloadGamepadButtons, "mReloadGamepadButtons").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc([](WeaponInstance& weapon) -> Weapon*
		{
			if (!weapon.mRuntimeWeapon)
			{
				LOG(LogAbilitySystem, Error, "GetRuntimeWeapon - mRuntimeWeapon was not initialized.");
				return nullptr;
			}
			return &weapon.mRuntimeWeapon.value();

		}, "GetRuntimeWeapon", MetaFunc::ExplicitParams<WeaponInstance&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

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

	metaType.AddFunc([](const WeaponInstance& weapon, const PlayerComponent& playerData)
	{
		return weapon.mReloadCounter == 0.0f &&
			(AbilitySystem::CheckKeyboardInput<&Input::IsKeyboardKeyHeld>(weapon.mKeyboardKeys) ||
			AbilitySystem::CheckGamepadInput<&Input::IsGamepadButtonHeld>(weapon.mGamepadButtons, playerData.mID));

	}, "IsPlayerShooting", MetaFunc::ExplicitParams<const WeaponInstance&, const PlayerComponent&>{}).GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc(&WeaponInstance::ResetCooldownAndAmmo, "ResetCooldownAndAmmo").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sCallFromEditorTag);
	metaType.AddFunc(&WeaponInstance::InitializeRuntimeWeapon, "InitializeRuntimeWeapon").GetProperties().Add(Props::sIsScriptableTag);

	ReflectFieldType<WeaponInstance>(metaType);
	return metaType;
}
