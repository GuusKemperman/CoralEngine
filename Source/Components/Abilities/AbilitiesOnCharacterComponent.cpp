#include "Precomp.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"

#include "Components/PlayerComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Assets/Ability/Ability.h"
#include "Systems/AbilitySystem.h"
#include "World/World.h"

CE::MetaType CE::AbilitiesOnCharacterComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilitiesOnCharacterComponent>{}, "AbilitiesOnCharacterComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	
	metaType.AddField(&AbilitiesOnCharacterComponent::mAbilitiesToInput, "mAbilitiesToInput").GetProperties().Add(Props::sNoInspectTag).Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilitiesOnCharacterComponent::mWeaponsToInput, "mWeaponsToInput").GetProperties().Add(Props::sNoInspectTag).Add(Props::sIsScriptableTag);

	BindEvent(metaType, sBeginPlayEvent, &AbilitiesOnCharacterComponent::OnBeginPlay);

#ifdef EDITOR
	BindEvent(metaType, sInspectEvent, &AbilitiesOnCharacterComponent::OnInspect);
#endif // EDITOR

	ReflectComponentType<AbilitiesOnCharacterComponent>(metaType);

	return metaType;
}

void CE::AbilitiesOnCharacterComponent::OnBeginPlay(World& world, entt::entity entity)
{
	for (auto& ability : mAbilitiesToInput)
	{
		// Make all the cooldown abilities available on being play.
		if (ability.mAbilityAsset->mRequirementType == Ability::Cooldown)
		{
			ability.mRequirementCounter = ability.mAbilityAsset->mRequirementToUse;
		}
		const MetaType* scriptType = MetaManager::Get().TryGetType(ability.mAbilityAsset->mOnAbilityActivateScript.GetMetaData().GetName());

		if (scriptType != nullptr)
		{
			world.GetRegistry().AddComponent(*scriptType, entity);
		}
	}
}

#ifdef EDITOR
static bool isPlayer = true; // Little hack to inspect the component conditionally
void CE::AbilitiesOnCharacterComponent::OnInspect(World& world, const std::vector<entt::entity>& entities)
{
	auto& reg = world.GetRegistry();
	if (entities.size() > 1)
	{
		ImGui::TextWrapped("Cannot inspect more than one Abilities On Character Component at a time.");
		return;
	}
	auto& abilities = reg.Get<AbilitiesOnCharacterComponent>(entities[0]);
	const auto player = reg.TryGet<PlayerComponent>(entities[0]);
	isPlayer = player != nullptr;
	ShowInspectUI("Abilities", abilities.mAbilitiesToInput);
	ShowInspectUI("Weapons", abilities.mWeaponsToInput);
}

void CE::AbilityInstance::DisplayWidget()
{
	ShowInspectUI("mAbilityAsset", mAbilityAsset);
	ShowInspectUIReadOnly("mRequirementCounter", mRequirementCounter);
	ShowInspectUIReadOnly("mChargesCounter", mChargesCounter);
	if (isPlayer)
	{
		ShowInspectUI("mKeyboardKeys", mKeyboardKeys);
		ShowInspectUI("mGamepadButtons", mGamepadButtons);
	}
}

void CE::WeaponInstance::DisplayWidget()
{
	ShowInspectUI("mWeaponAsset", mAbilityAsset);
	ShowInspectUIReadOnly("mRequirementCounter", mRequirementCounter);
	ShowInspectUIReadOnly("mChargesCounter", mChargesCounter);
	ShowInspectUIReadOnly("mTimeBetweenShotsCounter", mTimeBetweenShotsCounter);
	ShowInspectUI("mAmmoConsumption", mAmmoConsumption);
	if (isPlayer)
	{
		ShowInspectUI("mKeyboardKeys", mKeyboardKeys);
		ShowInspectUI("mGamepadButtons", mGamepadButtons);
	}
}
#endif // EDITOR
