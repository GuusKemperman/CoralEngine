#include "Precomp.h"
#include "Components/Abilities/AbilitiesOnPlayerComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Meta/ReflectedTypes/STD/ReflectSmartPtr.h"
#include "Assets/Ability.h"
#include "World/World.h"

Engine::MetaType Engine::AbilitiesOnPlayerComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilitiesOnPlayerComponent>{}, "AbilitiesOnPlayerComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	
	metaType.AddField(&AbilitiesOnPlayerComponent::mIsPlayer, "mIsPlayer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&AbilitiesOnPlayerComponent::mAbilitiesToInput, "mAbilitiesToInput").GetProperties().Add(Props::sNoInspectTag);

	BindEvent(metaType, sInspectEvent, &AbilitiesOnPlayerComponent::OnInspect);

	ReflectComponentType<AbilitiesOnPlayerComponent>(metaType);

	return metaType;
}

static bool isPlayer = true; // little hack to inspect the component conditionally
void Engine::AbilitiesOnPlayerComponent::OnInspect(World& world, const std::vector<entt::entity>& entities)
{
	auto& reg = world.GetRegistry();
	for (auto entity : entities)
	{
		auto& abilities = reg.Get<AbilitiesOnPlayerComponent>(entity);
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
}

bool Engine::AbilityInstanceWithInputs::operator==(const AbilityInstanceWithInputs& other) const
{
	return mAbilityAsset == other.mAbilityAsset &&
		mKeyboardKeys == other.mKeyboardKeys && 
		mGamepadButtons == other.mGamepadButtons;
}

bool Engine::AbilityInstanceWithInputs::operator!=(const AbilityInstanceWithInputs& other) const
{
	return mAbilityAsset != other.mAbilityAsset ||
		mKeyboardKeys != other.mKeyboardKeys ||
		mGamepadButtons != other.mGamepadButtons;
}

#ifdef EDITOR
void Engine::AbilityInstanceWithInputs::DisplayWidget()
{
	ShowInspectUI("mAbilityAsset", mAbilityAsset);
	ImGui::Text("mRequirementCounter: %f", mRequirementCounter);
	ImGui::Text("mChargesCounter: %d", mChargesCounter);
	if (isPlayer)
	{
		ShowInspectUI("mKeyboardKeys", mKeyboardKeys);
		ShowInspectUI("mGamepadButtons", mGamepadButtons);
	}
}
#endif // EDITOR

Engine::MetaType Engine::AbilityInstanceWithInputs::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityInstanceWithInputs>{}, "AbilityInstanceWithInputs" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&AbilityInstanceWithInputs::mAbilityAsset, "mAbilityAsset").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstanceWithInputs::mRequirementCounter, "mRequirementCounter").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstanceWithInputs::mChargesCounter, "mChargesCounter").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&AbilityInstanceWithInputs::mKeyboardKeys, "mKeyboardKeys").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityInstanceWithInputs::mGamepadButtons, "mGamepadButtons").GetProperties().Add(Props::sIsScriptableTag);

	return metaType;
}
