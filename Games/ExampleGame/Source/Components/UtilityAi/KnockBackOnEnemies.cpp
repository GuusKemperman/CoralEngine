#include "Precomp.h"
#include "Components/UtililtyAi/KnockBackOnEnemies.h"

#include "Components/Abilities/AbilitiesOnCharacterComponent.h"
#include "Components/UtililtyAi/States/KnockBackState.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"
#include "World/World.h"

void KnockBackOnEnemies::OnAbilityHit(CE::World& world, const entt::entity castBy, const entt::entity target, entt::entity)
{
	auto* knockBackState = world.GetRegistry().TryGet<Game::KnockBackState>(target);

	if (knockBackState == nullptr)
	{
		LOG(LogAI, Warning, "Kockback State - enemy {} does not have a KnockBackState.", entt::to_integral(target));
		return;
	}

	auto abilities = world.GetRegistry().TryGet<CE::AbilitiesOnCharacterComponent>(castBy);
	if (abilities == nullptr)
	{
		LOG(LogAI, Warning, "KnockBackState - entity {} does not have an AbilitiesOnCharacterComponent.", entt::to_integral(castBy));
		return;		
	}
	if (abilities->mWeaponsToInput.empty())
	{
		LOG(LogAI, Warning, "KnockBackState - entity {} does not have any weapons.", entt::to_integral(castBy));
		return;		
	}
	auto& weapon = abilities->mWeaponsToInput[0].mRuntimeWeapon;
	if (weapon.has_value() == false)
	{
		LOG(LogAI, Warning, "KnockBackState - runtime weapon no initialized.", entt::to_integral(castBy));
		return;
	}

	knockBackState->Initialize(weapon->mKnockback);
}

CE::MetaType KnockBackOnEnemies::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<KnockBackOnEnemies>{}, "KnockBackOnEnemies" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAbilityHitEvent, &KnockBackOnEnemies::OnAbilityHit);

	CE::ReflectComponentType<KnockBackOnEnemies>(type);
	return type;
}