#include "Precomp.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"
#include "Utilities/Imgui/ImguiInspect.h"
#include "Assets/Ability.h"
#include "Systems/AbilitySystem.h"
#include "World/World.h"

CE::MetaType CE::AbilitiesOnCharacterComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilitiesOnCharacterComponent>{}, "AbilitiesOnCharacterComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	
	metaType.AddField(&AbilitiesOnCharacterComponent::mIsPlayer, "mIsPlayer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&AbilitiesOnCharacterComponent::mAbilitiesToInput, "mAbilitiesToInput").GetProperties().Add(Props::sNoInspectTag).Add(Props::sIsScriptableTag);

	BindEvent(metaType, sBeginPlayEvent, &AbilitiesOnCharacterComponent::OnBeginPlay);

#ifdef EDITOR
	BindEvent(metaType, sInspectEvent, &AbilitiesOnCharacterComponent::OnInspect);
#endif // EDITOR

	ReflectComponentType<AbilitiesOnCharacterComponent>(metaType);

	return metaType;
}

void CE::AbilitiesOnCharacterComponent::OnBeginPlay(World&, entt::entity)
{
	for (auto& ability : mAbilitiesToInput)
	{
		// Make all the cooldown abilities available on being play.
		if (ability.mAbilityAsset->mRequirementType == Ability::Cooldown)
		{
			ability.mRequirementCounter = ability.mAbilityAsset->mRequirementToUse;
		}
	}
}

#ifdef EDITOR
static bool isPlayer = true; // little hack to inspect the component conditionally
void CE::AbilitiesOnCharacterComponent::OnInspect(World& world, const std::vector<entt::entity>& entities)
{
	auto& reg = world.GetRegistry();
	if (entities.size() > 1)
	{
		ImGui::TextWrapped("Cannot inspect more than one Abilities On Character Component at a time.");
		return;
	}
	auto& abilities = reg.Get<AbilitiesOnCharacterComponent>(entities[0]);
	ShowInspectUI("mIsPlayer", abilities.mIsPlayer);
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::Text("Check this box if the entity this component is attached to is a player.\nThis defines whether an ability needs input.");
		ImGui::EndTooltip();
	}
	isPlayer = abilities.mIsPlayer;
	ShowInspectUI("Abilities", abilities.mAbilitiesToInput);
}
#endif // EDITOR

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

#ifdef EDITOR
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
#endif // EDITOR

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

	ReflectFieldType<AbilityInstance>(metaType);
	return metaType;
}
