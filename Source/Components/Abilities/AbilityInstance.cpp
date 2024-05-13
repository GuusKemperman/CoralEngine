#include "Precomp.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Assets/Ability/Ability.h"
#include "Systems/AbilitySystem.h"
#include "World/World.h"

void CE::AbilityInstance::MakeAbilityReadyToBeActivated()
{
	mRequirementCounter = mAbilityAsset->mRequirementToUse;
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

			metaType.AddFunc([](AbilityInstance& ability)
				{
					ability.MakeAbilityReadyToBeActivated();
				}, "MakeAbilityReadyToBeActivated", MetaFunc::ExplicitParams<AbilityInstance&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

				ReflectFieldType<AbilityInstance>(metaType);
				return metaType;
}

bool CE::WeaponInstance::operator==(const WeaponInstance& other) const
{
	return AbilityInstance::operator==(other);
}

bool CE::WeaponInstance::operator!=(const WeaponInstance& other) const
{
	return AbilityInstance::operator!=(other);
}

CE::MetaType CE::WeaponInstance::Reflect()
{

	MetaType metaType = MetaType{ MetaType::T<WeaponInstance>{}, "WeaponInstance", MetaType::Base<AbilityInstance>{} };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	//metaType.AddField(MetaManager::Get().GetType<AssetHandle<Ability>>(), static_cast<uint32>(offsetof(WeaponInstance, mAbilityAsset)), "mWeaponAsset").GetProperties().Add(Props::sIsScriptableTag);
	//metaType.AddField(MetaManager::Get().GetType<float>(), static_cast<uint32>(offsetof(WeaponInstance, mRequirementCounter)), "mCooldownCounter").GetProperties().Add(Props::sIsScriptableTag);
	//metaType.AddField(MetaManager::Get().GetType<int>(), static_cast<uint32>(offsetof(WeaponInstance, mChargesCounter)), "mChargesCounter").GetProperties().Add(Props::sIsScriptableTag);

	//metaType.AddField(MetaManager::Get().GetType<Input::KeyboardKey>(), static_cast<uint32>(offsetof(WeaponInstance, mKeyboardKeys)), "mKeyboardKeys").GetProperties().Add(Props::sIsScriptableTag);
	//metaType.AddField(MetaManager::Get().GetType<Input::GamepadButton>(), static_cast<uint32>(offsetof(WeaponInstance, mGamepadButtons)), "mGamepadButtons").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&WeaponInstance::mTimeBetweenShotsCounter, "mTimeBetweenShotsCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&WeaponInstance::mAmmoConsumption, "mAmmoConsumption").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc([](WeaponInstance& ability, entt::entity castBy, CharacterComponent& characterData)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			return AbilitySystem::ActivateAbility(*world, castBy, characterData, ability);

		}, "ActivateWeapon", MetaFunc::ExplicitParams<WeaponInstance&, entt::entity, CharacterComponent&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

		metaType.AddFunc([](const WeaponInstance& ability, const CharacterComponent& characterData)
			{
				return AbilitySystem::CanAbilityBeActivated(characterData, ability);

			}, "CanWeaponBeActivated", MetaFunc::ExplicitParams<const WeaponInstance&, const CharacterComponent&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

			metaType.AddFunc([](WeaponInstance& ability)
				{
					ability.MakeAbilityReadyToBeActivated();
				}, "MakeWeaponReadyToBeActivated", MetaFunc::ExplicitParams<WeaponInstance&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

				ReflectFieldType<WeaponInstance>(metaType);
				return metaType;
}
