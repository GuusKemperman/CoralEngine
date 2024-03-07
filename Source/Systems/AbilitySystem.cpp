#include "Precomp.h"
#include "Systems/AbilitySystem.h"

#include "Assets/Ability.h"
#include "Components/Abilities/AbilitiesOnPlayerComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaFunc.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Core/Input.h"
#include "Assets/Script.h"

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
                    if (ability.mRequirementCounter < ability.mAbilityAsset->mRequirementToUse)
                    {
                        ability.mRequirementCounter += dt;
                    }
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
                            ActivateAbility(world, entity, characterData, ability);
                        }
                    }
                    for (auto& button : ability.mGamepadButtons)
                    {
                        // TODO: replace zero with player id by separating abilities on player and abilities on AI
                        if (input.WasGamepadButtonPressed(0, button))
                        {
                            ActivateAbility(world, entity, characterData, ability);
                        }
                    }
                }
	        }
        }
    }
}

void Engine::AbilitySystem::ActivateAbility(World& world, entt::entity castBy, CharacterComponent& characterData, AbilityInstanceWithInputs& ability)
{
    // on ability activate effect
    
    if (auto metaType = MetaManager::Get().TryGetType(ability.mAbilityAsset->mScript->GetName()))
    {
	    if (auto metaFunc = TryGetEvent(*metaType, sAbilityActivateEvent))
        {
            (*metaFunc)(world, castBy);
        }
    }
    else
    {
        LOG(LogAbilitySystem, Error, "Unable to call OnAbilityActivate event for ability "{}"", ability.mAbilityAsset->GetName())
    }
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
