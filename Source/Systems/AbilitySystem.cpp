#include "Precomp.h"
#include "Systems/AbilitySystem.h"

#include "Assets/Ability.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaFunc.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Core/Input.h"
#include "Assets/Script.h"
#include "Components/Abilities/AOEComponent.h"
#include "Components/Abilities/EffectsOnCharacterComponent.h"
#include "Components/Abilities/ProjectileComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Utilities/AbilityFunctionality.h"

void Engine::AbilitySystem::Update(World& world, float dt)
{
    Registry& reg = world.GetRegistry();

    auto viewProjectiles = reg.View<ProjectileComponent>();
    for (auto [entity, projectile] : viewProjectiles.each())
    {
        auto& physicsBody = reg.Get<PhysicsBody2DComponent>(entity);
        projectile.mCurrentRange += glm::length(physicsBody.mLinearVelocity) * dt;
	    if (projectile.mCurrentRange >= projectile.mRange)
	    {
            reg.Destroy(entity, true);
	    }
    }

    auto viewAOE = reg.View<AOEComponent>();
    for (auto [entity, aoe] : viewAOE.each())
    {
        aoe.mDurationTimer += dt;
        if (aoe.mDurationTimer >= aoe.mDuration)
        {
            reg.Destroy(entity, true);
        }
    }

    auto viewCharacters = reg.View<CharacterComponent, EffectsOnCharacterComponent>();
    const auto& input = Input::Get();
    for (auto [entity, characterData, effects] : viewCharacters.each())
    {
        // update durational effects
        auto& durationalEffects = effects.mDurationalEffects;
        for (auto it = durationalEffects.begin(); it != durationalEffects.end();)
        {
            it->mDurationTimer += dt;
            if (it->mDurationTimer >= it->mDuration)
            {
                AbilityFunctionality::RevertDurationalEffect(characterData, *it);
                it = durationalEffects.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // update over time effects
        auto& overTimeEffects = effects.mOverTimeEffects;
        for (auto it = overTimeEffects.begin(); it != overTimeEffects.end();)
        {
            it->mDurationTimer += dt;
            if (it->mDurationTimer >= it->mDuration)
            {
                it->mTicksCounter++;
            	it->mDurationTimer = 0.f;
                AbilityFunctionality::ApplyInstantEffectForOverTimeEffect(world, entity, it->mEffectSettings.mStat, it->mEffectSettings.mAmount, it->mEffectSettings.mFlatOrPercentage, it->mEffectSettings.mIncreaseOrDecrease, it->mDealtDamageModifierOfCastByCharacter);
            }
            if (it->mTicksCounter >= it->mTicks)
            {
                it = overTimeEffects.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // update GDC
        if (characterData.mGlobalCooldownTimer > 0.f)
        {
            characterData.mGlobalCooldownTimer -= dt;
        }

        // abilities
        auto abilities = reg.TryGet<AbilitiesOnCharacterComponent>(entity);
        if (abilities == nullptr)
        {
	        continue;
        }

        // create abilities
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
            if (CanAbilityBeActivated(characterData, ability))
            {
                if (abilities->mIsPlayer) // player
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

bool Engine::AbilitySystem::CanAbilityBeActivated(const CharacterComponent& characterData, const AbilityInstance& ability)
{
    return ability.mRequirementCounter >= ability.mAbilityAsset->mRequirementToUse &&
        ability.mChargesCounter < ability.mAbilityAsset->mCharges &&
        (ability.mAbilityAsset->mGlobalCooldown == false || characterData.mGlobalCooldownTimer <= 0.f);
}

void Engine::AbilitySystem::ActivateAbility(World& world, entt::entity castBy, CharacterComponent& characterData, AbilityInstance& ability)
{
    if (!CanAbilityBeActivated(characterData, ability))
    {
        // for the player, this will get checked twice,
        // but it is a small tradeoff for safety in the AI usage
        return;
    }

    // ability activate event
    if (auto metaType = MetaManager::Get().TryGetType(ability.mAbilityAsset->mScript->GetName()))
    {
	    if (auto metaFunc = TryGetEvent(*metaType, sAbilityActivateEvent))
        {
            metaFunc->InvokeUncheckedUnpacked(world, castBy);
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
