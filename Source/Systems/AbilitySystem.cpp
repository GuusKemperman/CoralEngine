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

CE::AbilitySystem::AbilitySystem()
{
    sAbilityActivateEvents = CE::GetAllBoundEvents(CE::sAbilityActivateEvent);
    sReloadStartedEvents = CE::GetAllBoundEvents(CE::sReloadStartedEvent);
    sReloadCompletedEvents = CE::GetAllBoundEvents(CE::sReloadCompletedEvent);
    sReloadInterruptedEvents = CE::GetAllBoundEvents(CE::sReloadInterruptedEvent);
    sEnemyKilledEvents = CE::GetAllBoundEvents(CE::sEnemyKilledEvent);
    sGettingHitEvents = CE::GetAllBoundEvents(CE::sGettingHitEvent);
    sAbilityHitEvents = CE::GetAllBoundEvents(CE::sAbilityHitEvent);
    sCritEvents = CE::GetAllBoundEvents(CE::sCritEvent);
}

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
                if (CheckKeyboardInput<&Input::IsKeyboardKeyHeld>(ability.mKeyboardKeys) ||
                    CheckGamepadInput<&Input::IsGamepadButtonHeld>(ability.mGamepadButtons, playerComponent->mID))
                {
                    ActivateAbility(world, entity, characterData, ability);
                }
            }
        }
    }
}

void CE::AbilitySystem::UpdateWeaponsVector(AbilitiesOnCharacterComponent& abilities, CharacterComponent& characterData, entt::entity entity, World& world, float dt)
{
    for (auto& weapon : abilities.mWeaponsToInput)
    {
        if (weapon.mWeaponAsset == nullptr || weapon.mRuntimeWeapon.has_value() == false)
        {
            continue;
        }

        // Shot Delay counter
        weapon.mShotDelayCounter = std::min(weapon.mShotDelayCounter + dt * weapon.mRuntimeWeapon->mFireSpeed, weapon.mRuntimeWeapon->mShotDelay);

    	// Reload counter
        bool reloadCompleted{}; // Because it should only be called for the player.
    	if (weapon.mReloadCounter > 0.f)
        {
	    	weapon.mReloadCounter = std::max(weapon.mReloadCounter - dt * weapon.mRuntimeWeapon->mReloadSpeed, 0.f);
            if (weapon.mReloadCounter == 0.f)
            {
                // Reload completed
                weapon.mAmmoCounter = weapon.mRuntimeWeapon->mCharges;
                reloadCompleted = true;
            }
        }

        if (auto playerComponent = world.GetRegistry().TryGet<PlayerComponent>(entity))
        {
            // Reload
            if (reloadCompleted)
            {
                CallBoundEventsWithNoExtraParams(world, entity, sReloadCompletedEvents);
            }
            if ((CheckKeyboardInput<&Input::WasKeyboardKeyPressed>(weapon.mReloadKeyboardKeys) ||
                CheckGamepadInput<&Input::WasGamepadButtonPressed>(weapon.mReloadGamepadButtons, playerComponent->mID)) &&
                weapon.mReloadCounter == 0.f)
            {
                // Trigger reload
                weapon.mReloadCounter = weapon.mRuntimeWeapon->mRequirementToUse;
                CallBoundEventsWithNoExtraParams(world, entity, sReloadStartedEvents);
            }
            if ((CheckKeyboardInput<&Input::WasKeyboardKeyPressed>(weapon.mKeyboardKeys) ||
                CheckGamepadInput<&Input::WasGamepadButtonPressed>(weapon.mGamepadButtons, playerComponent->mID)) &&
                weapon.mAmmoCounter > 0
                && weapon.mReloadCounter != 0.0f)
            {
                // Reload interrupted
                weapon.mReloadCounter = 0.f;
                CallBoundEventsWithNoExtraParams(world, entity, sReloadInterruptedEvents);
            }

            // Activate abilities for the player based on input
            if (CanWeaponBeActivated(characterData, weapon))
            {
                if (weapon.mRuntimeWeapon->mShootOnRelease == false)
                {
                    if (CheckKeyboardInput<&Input::IsKeyboardKeyHeld>(weapon.mKeyboardKeys) || 
                        CheckGamepadInput<&Input::IsGamepadButtonHeld>(weapon.mGamepadButtons, playerComponent->mID))
                    {
                        ActivateWeapon(world, entity, characterData, weapon);
                    }
                }
                else
                {
                    if (CheckKeyboardInput<&Input::WasKeyboardKeyReleased>(weapon.mKeyboardKeys) ||
                        CheckGamepadInput<&Input::WasGamepadButtonReleased>(weapon.mGamepadButtons, playerComponent->mID))
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
    characterData.mGlobalCooldownTimer = characterData.mGlobalCooldown;
    ability.mChargesCounter--;
    if (ability.mChargesCounter <= 0)
    {
        ability.mChargesCounter = ability.mAbilityAsset->mCharges;
        ability.mRequirementCounter = ability.mAbilityAsset->mRequirementToUse;
    }
    if (ability.mAbilityAsset->mOnAbilityActivateScript != nullptr)
    {
        if (auto metaType = MetaManager::Get().TryGetType(ability.mAbilityAsset->mOnAbilityActivateScript.GetMetaData().GetName());
            metaType == nullptr)
        {
            LOG(LogAbilitySystem, Error, "Did not find script {} when trying to activate ability {}",
                ability.mAbilityAsset->mOnAbilityActivateScript.GetMetaData().GetName(),
                ability.mAbilityAsset.GetMetaData().GetName());
        }
        CallBoundEventsWithNoExtraParams(world, castBy, sAbilityActivateEvents);
    }
    else
    {
        LOG(LogAbilitySystem, Error, "Ability {} does not have a script selected.", ability.mAbilityAsset.GetMetaData().GetName());
    }

    return true;
}

bool CE::AbilitySystem::CanWeaponBeActivated(const CharacterComponent& characterData, const WeaponInstance& weapon)
{
    if (weapon.mWeaponAsset == nullptr || weapon.mRuntimeWeapon.has_value() == false)
    {
        return false;
    }
    return ((weapon.mAmmoCounter > 0 && weapon.mRuntimeWeapon->mShootOnRelease == false) || 
        ((weapon.mShotsAccumulated > 0 || weapon.mAmmoCounter > 0) && weapon.mRuntimeWeapon->mShootOnRelease == true)) &&
        weapon.mShotDelayCounter >= weapon.mRuntimeWeapon->mShotDelay &&
        (weapon.mRuntimeWeapon->mGlobalCooldown == false || characterData.mGlobalCooldownTimer <= 0.f);
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
    characterData.mGlobalCooldownTimer = characterData.mGlobalCooldown;
    weapon.mShotDelayCounter = 0.f;
    if (weapon.mAmmoConsumption == true)
    {
        weapon.mAmmoCounter--;
    }
    if (weapon.mWeaponAsset->mOnAbilityActivateScript != nullptr)
    {
        if (auto metaType = MetaManager::Get().TryGetType(weapon.mWeaponAsset->mOnAbilityActivateScript.GetMetaData().GetName()); 
            metaType == nullptr)
        {
            LOG(LogAbilitySystem, Error, "Did not find script {} when trying to activate weapon {}",
                weapon.mWeaponAsset->mOnAbilityActivateScript.GetMetaData().GetName(),
                weapon.mWeaponAsset.GetMetaData().GetName());
        }
        CallBoundEventsWithNoExtraParams(world, castBy, sAbilityActivateEvents);
    }
    else
    {
        LOG(LogAbilitySystem, Error, "Weapon {} does not have a script selected.", weapon.mWeaponAsset.GetMetaData().GetName());
    }
    weapon.mReloadCounter = 0.f;
    if (weapon.mAmmoCounter <= 0)
    {
        // Trigger reload
        weapon.mReloadCounter = weapon.mRuntimeWeapon->mRequirementToUse;
        CallBoundEventsWithNoExtraParams(world, castBy, sReloadStartedEvents);
    }

    return true;
}

void CE::AbilitySystem::CallBoundEventsWithNoExtraParams(World& world, entt::entity castBy,
	const std::vector<CE::BoundEvent>& boundEvents)
{
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
