#include "Precomp.h"
#include "Systems/AbilitySystem.h"

#include "Components/PlayerComponent.h"
#include "Assets/Ability/Ability.h"
#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaFunc.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Core/Input.h"
#include "Assets/Script.h"
#include "Assets/Ability/Weapon.h"
#include "Components/MeshColorComponent.h"
#include "Components/TransformComponent.h"
#include "Components/Abilities/AbilityLifetimeComponent.h"
#include "Components/Abilities/EffectsOnCharacterComponent.h"
#include "Components/Abilities/ProjectileComponent.h"
#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Utilities/AbilityFunctionality.h"

void CE::AbilitySystem::Update(World& world, float dt)
{
    Registry& reg = world.GetRegistry();

    auto viewProjectiles = reg.View<ProjectileComponent>();
    for (auto [entity, projectile] : viewProjectiles.each())
    {
        auto& physicsBody = reg.Get<PhysicsBody2DComponent>(entity);
        projectile.mCurrentRange += glm::length(physicsBody.mLinearVelocity) * dt;
	    if (projectile.mCurrentRange >= projectile.mRange && projectile.mDestroyOnRangeReached)
	    {
            reg.Destroy(entity, true);
	    }
    }

    auto viewLifetime = reg.View<AbilityLifetimeComponent>();
    for (auto [entity, lifetime] : viewLifetime.each())
    {
        lifetime.mDurationTimer += dt;
        if (lifetime.mDurationTimer >= lifetime.mDuration)
        {
            reg.Destroy(entity, true);
        }
    }

    auto viewCharacters = reg.View<CharacterComponent, AbilitiesOnCharacterComponent, EffectsOnCharacterComponent>();
    for (auto [entity, characterData, abilities, effects] : viewCharacters.each())
    {
        // Durational effects
        std::vector<DurationalEffect>& durationalEffects = effects.mDurationalEffects;
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

        // Over time effects
        std::vector<OverTimeEffect>& overTimeEffects = effects.mOverTimeEffects;
        for (auto it = overTimeEffects.begin(); it != overTimeEffects.end();)
        {
            it->mDurationTimer += dt;
            if (it->mDurationTimer >= it->mTickDuration)
            {
                it->mTicksCounter++;
            	it->mDurationTimer = 0.f;
                AbilityFunctionality::ApplyInstantEffect(world, &it->mCastByCharacterData, entity, it->mEffectSettings);
            }
            if (it->mTicksCounter >= it->mNumberOfTicks)
            {
                it = overTimeEffects.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Visual effects
        std::vector<VisualEffect>& visualEffects = effects.mVisualEffects;
        if (visualEffects.empty() == false)
        {
            // Get the effect color
            glm::vec3 color{};
            for (auto it = visualEffects.begin(); it != visualEffects.end();)
            {
                color = it->mColor;
                it->mDurationTimer += dt;
                if (it->mDurationTimer >= it->mDuration)
                {
                    it = visualEffects.erase(it);
                    if (visualEffects.empty() == false)
                    {
                        color = visualEffects.back().mColor;
                        // Use the last visual effect in the vector,
                        // otherwise it will not have a color for one frame
                        // if the erased visual effect was the last in the vector.
                    }
                    else
                    {
                        color = {};
                    }
                }
                else
                {
                    ++it;
                }
            }

            bool changedMeshColor = false;

            // Set color for the entity if it has a MeshColorComponent
            auto meshColor = reg.TryGet<MeshColorComponent>(entity);
            if (meshColor != nullptr)
            {
                meshColor->mColorAddition = color;
                changedMeshColor = true;
            }

            // Set mesh color for all children that have a MeshColorComponent
            const TransformComponent* transform = reg.TryGet<TransformComponent>(entity);
            if (transform == nullptr)
            {
                LOG(LogAbilitySystem, Error, "Character with entity id {} does not have a TransformComponent attached.", entt::to_integral(entity));
            }
            else
            {
                const auto setMeshColor = [&reg, &color, &changedMeshColor](const auto& self, const TransformComponent& current) -> void
                    {
                        for (const TransformComponent& child : current.GetChildren())
                        {
                            MeshColorComponent* const meshColor = reg.TryGet<MeshColorComponent>(child.GetOwner());
                            if (meshColor != nullptr)
                            {
                                meshColor->mColorAddition = color;
                                changedMeshColor = true;
                            }
                            self(self, child);
                        }
                    };
                setMeshColor(setMeshColor, *transform);
            }

            if (changedMeshColor == false)
            {
                LOG(LogAbilitySystem, Error, "Character with entity id {} or any of its children do not have a MeshColorComponent attached - visual effects of abilities cannot be displayed.", entt::to_integral(entity));
            }
        }

        // Update GDC
        characterData.mGlobalCooldownTimer = std::max(characterData.mGlobalCooldownTimer - dt, 0.0f);

        UpdateAbilitiesVector(abilities, characterData, entity, world, dt);
        UpdateWeaponsVector(abilities, characterData, entity, world, dt);
    }
}

void CE::AbilitySystem::UpdateAbilitiesVector(AbilitiesOnCharacterComponent& abilities, CharacterComponent& characterData, entt::entity entity, World& world, float dt)
{
    const auto& input = Input::Get();
    for (auto& ability : abilities.mAbilitiesToInput)
    {
        if (ability.mAbilityAsset == nullptr)
        {
            continue;
        }

        // Update counter
        switch (ability.mAbilityAsset->mRequirementType)
        {
        case Ability::Cooldown:
        {
            ability.mRequirementCounter = std::max(ability.mRequirementCounter - dt, 0.f);
            break;
        }
        case Ability::Mana:
        {
            // Custom functionality
            break;
        }
        }
        // Activate abilities for the player based on input
        if (auto playerComponent = world.GetRegistry().TryGet<PlayerComponent>(entity))
        {
            if (CanAbilityBeActivated(characterData, ability))
            {
                for (auto& key : ability.mKeyboardKeys)
                {
                    if (input.IsKeyboardKeyHeld(key))
                    {
                        ActivateAbility(world, entity, characterData, ability);
                    }
                }
                for (auto& button : ability.mGamepadButtons)
                {
                    if (input.IsGamepadButtonHeld(playerComponent->mID, button))
                    {
                        ActivateAbility(world, entity, characterData, ability);
                    }
                }
            }
        }
    }
}

void CE::AbilitySystem::UpdateWeaponsVector(AbilitiesOnCharacterComponent& abilities, CharacterComponent& characterData, entt::entity entity, World& world, float dt)
{
    const auto& input = Input::Get();
    for (auto& weapon : abilities.mWeaponsToInput)
    {
        if (weapon.mWeaponAsset == nullptr)
        {
            continue;
        }

        // Update counters
        weapon.mReloadCounter = std::max(weapon.mReloadCounter - dt * weapon.mWeaponAsset->mReloadSpeed, 0.f);
        weapon.mTimeBetweenShotsCounter = std::min(weapon.mTimeBetweenShotsCounter + dt * weapon.mWeaponAsset->mFireSpeed, weapon.mWeaponAsset->mTimeBetweenShots);

        // Activate abilities for the player based on input
        if (auto playerComponent = world.GetRegistry().TryGet<PlayerComponent>(entity))
        {
            if (CanWeaponBeActivated(characterData, weapon))
            {
                for (auto& key : weapon.mKeyboardKeys)
                {
                    if (input.IsKeyboardKeyHeld(key))
                    {
                        ActivateWeapon(world, entity, characterData, weapon);
                    }
                }
                for (auto& button : weapon.mGamepadButtons)
                {
                    if (input.IsGamepadButtonHeld(playerComponent->mID, button))
                    {
                        ActivateWeapon(world, entity, characterData, weapon);
                    }
                }
            }
        }
    }
}

bool CE::AbilitySystem::CanAbilityBeActivated(const CharacterComponent& characterData, const AbilityInstance& ability)
{
    if (ability.mAbilityAsset == nullptr)
    {
        return false;
    }
    return ability.mRequirementCounter <= 0.f &&
        ability.mChargesCounter > 0 &&
        (ability.mAbilityAsset->mGlobalCooldown == false || characterData.mGlobalCooldownTimer <= 0.f);
}

bool CE::AbilitySystem::ActivateAbility(World& world, entt::entity castBy, CharacterComponent& characterData, AbilityInstance& ability)
{
    if (!CanAbilityBeActivated(characterData, ability))
    {
        // For the player, this will get checked twice,
        // but it is a small tradeoff for safety in the AI usage.
        return false;
    }

    // Ability activate event
    if (ability.mAbilityAsset->mOnAbilityActivateScript != nullptr)
    {
        if (auto metaType = MetaManager::Get().TryGetType(ability.mAbilityAsset->mOnAbilityActivateScript.GetMetaData().GetName());
            metaType == nullptr)
        {
            LOG(LogAbilitySystem, Error, "Did not find script {} when trying to activate ability {}",
                ability.mAbilityAsset->mOnAbilityActivateScript.GetMetaData().GetName(),
                ability.mAbilityAsset.GetMetaData().GetName());
        }
        CallAllOnAbilityActivateEvents(world, castBy);
    }
    else
    {
        LOG(LogAbilitySystem, Error, "Ability {} does not have a script selected.", ability.mAbilityAsset.GetMetaData().GetName());
    }
    characterData.mGlobalCooldownTimer = characterData.mGlobalCooldown;
    ability.mChargesCounter--;
    if (ability.mChargesCounter <= 0)
    {
        ability.mChargesCounter = ability.mAbilityAsset->mCharges;
        ability.mRequirementCounter = ability.mAbilityAsset->mRequirementToUse;
    }

    return true;
}

bool CE::AbilitySystem::CanWeaponBeActivated(const CharacterComponent& characterData, const WeaponInstance& weapon)
{
    if (weapon.mWeaponAsset == nullptr)
    {
        return false;
    }
    return weapon.mReloadCounter <= 0.f &&
        weapon.mAmmoCounter > 0 &&
        weapon.mTimeBetweenShotsCounter >= weapon.mWeaponAsset->mTimeBetweenShots &&
        (weapon.mWeaponAsset->mGlobalCooldown == false || characterData.mGlobalCooldownTimer <= 0.f);
}

bool CE::AbilitySystem::ActivateWeapon(World& world, entt::entity castBy, CharacterComponent& characterData,
	WeaponInstance& weapon)
{
    if (!CanWeaponBeActivated(characterData, weapon))
    {
        // For the player, this will get checked twice,
        // but it is a small tradeoff for safety in the AI usage.
        return false;
    }

    // Ability activate event
    if (weapon.mWeaponAsset->mOnAbilityActivateScript != nullptr)
    {
        if (auto metaType = MetaManager::Get().TryGetType(weapon.mWeaponAsset->mOnAbilityActivateScript.GetMetaData().GetName()); 
            metaType == nullptr)
        {
            LOG(LogAbilitySystem, Error, "Did not find script {} when trying to activate weapon {}",
                weapon.mWeaponAsset->mOnAbilityActivateScript.GetMetaData().GetName(),
                weapon.mWeaponAsset.GetMetaData().GetName());
        }
        CallAllOnAbilityActivateEvents(world, castBy);
    }
    else
    {
        LOG(LogAbilitySystem, Error, "Weapon {} does not have a script selected.", weapon.mWeaponAsset.GetMetaData().GetName());
    }
    characterData.mGlobalCooldownTimer = characterData.mGlobalCooldown;
    weapon.mTimeBetweenShotsCounter = 0.f;
    if (weapon.mAmmoConsumption == true)
    {
        weapon.mAmmoCounter--;
    }
    if (weapon.mAmmoCounter <= 0)
    {
        weapon.mAmmoCounter = weapon.mWeaponAsset->mCharges;
        weapon.mReloadCounter = weapon.mWeaponAsset->mRequirementToUse;
    }

    return true;
}

void CE::AbilitySystem::CallAllOnAbilityActivateEvents(World& world, entt::entity castBy)
{
    const std::vector<BoundEvent> boundEvents = GetAllBoundEvents(sAbilityActivateEvent);
    for (const BoundEvent& boundEvent : boundEvents)
    {
        entt::sparse_set* const storage = world.GetRegistry().Storage(boundEvent.mType.get().GetTypeId());

        if (storage == nullptr
            || !storage->contains(castBy))
        {
            continue;
        }

        if (boundEvent.mIsStatic)
        {
            boundEvent.mFunc.get().InvokeUncheckedUnpacked(world, castBy);
        }
        else
        {
            MetaAny component{ boundEvent.mType, storage->value(castBy), false };
            boundEvent.mFunc.get().InvokeUncheckedUnpacked(component, world, castBy);
        }
    }
}

CE::MetaType CE::AbilitySystem::Reflect()
{
	return MetaType{ MetaType::T<AbilitySystem>{}, "AbilitySystem", MetaType::Base<System>{} };
}
