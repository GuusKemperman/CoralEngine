#include "Precomp.h"
#include "Systems/AbilitySystem.h"

#include "Assets/Ability.h"
#include "Components/Abilities/AbilitiesOnPlayerComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Core/Input.h"

void Engine::AbilitySystem::Update(World& world, float dt)
{
    Registry& reg = world.GetRegistry();
    auto view = reg.View<CharacterComponent>();
    const auto& input = Input::Get();
    for (auto [entity, characterData] : view.each())
    {
        if (characterData.mGlobalCooldownTimer > 0.f)
        {
            characterData.mGlobalCooldownTimer -= dt;
        }

        if (auto abilities = reg.TryGet<AbilitiesOnPlayerComponent>(entity); abilities)
        {
	        for (auto& ability : abilities->mAbilitiesToInput)
	        {
                // update counter
		        switch (ability.mAbilityAsset->mRequirementType)
		        {
		        case Ability::Cooldown:
                {
                    ability.mRequirementCounter += dt;
                    break;
                }
                case Ability::Mana:
                {
                    break;
                }
		        }
                // check if ability can be used
                if (ability.mRequirementCounter >= ability.mAbilityAsset->mRequirementToUse && 
                    ability.mChargesCounter < ability.mAbilityAsset->mCharges && 
                    (ability.mAbilityAsset->mGlobalCooldown == false || characterData.mGlobalCooldownTimer <= 0.f))
                {
	                for (auto& key : ability.mKeyboardKeys)
	                {
		                if (input.WasKeyboardKeyPressed(key))
                        {
                            DeployAbility(characterData, ability);
                        }
                    }
                    for (auto& button : ability.mGamepadButtons)
                    {
                        // TODO: replace zero with player id by separating abilities on player and abilities on AI
                        if (input.WasGamepadButtonPressed(0, button))
                        {
                            DeployAbility(characterData, ability);
                        }
                    }
                }
	        }
        }
    }
}

void Engine::AbilitySystem::DeployAbility(CharacterComponent& characterData, AbilityInstanceWithInputs& ability)
{
    // on fire event
    LOG(LogAbilitySystem, Message, "Fired ability {}", ability.mAbilityAsset->GetName())
    characterData.mGlobalCooldownTimer = characterData.mGlobalCooldown;
    ability.mChargesCounter++;
    if (ability.mChargesCounter >= ability.mAbilityAsset->mCharges)
    {
        ability.mChargesCounter = 0;
        ability.mRequirementCounter = 0;
    }
}

Engine::MetaType Engine::AbilitySystem::Reflect()
{
	return MetaType{ MetaType::T<AbilitySystem>{}, "AbilitySystem", MetaType::Base<System>{} };
}
