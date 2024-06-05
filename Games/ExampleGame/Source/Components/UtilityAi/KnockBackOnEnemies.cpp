#include "Precomp.h"
#include "Components/UtililtyAi/KnockBackOnEnemies.h"

#include "Components/UtililtyAi/States/KnockBackState.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"
#include "World/World.h"

void KnockBackOnEnemies::OnAbilityHit(CE::World& world, const entt::entity, const entt::entity target, entt::entity)
{
	auto* knockBackState = world.GetRegistry().TryGet<Game::KnockBackState>(target);

	if (knockBackState == nullptr)
	{
		LOG(LogAI, Warning, "Dash State - enemy {} does not have a PhysicsBody2D Component.", entt::to_integral(target));
		return;
	}

	knockBackState->ResetTimer();
}

CE::MetaType KnockBackOnEnemies::Reflect()
{
	auto type = CE::MetaType{ CE::MetaType::T<KnockBackOnEnemies>{}, "KnockBackOnEnemies" };
	type.GetProperties().Add(CE::Props::sIsScriptableTag);

	BindEvent(type, CE::sAbilityHitEvent, &KnockBackOnEnemies::OnAbilityHit);

	CE::ReflectComponentType<KnockBackOnEnemies>(type);
	return type;
}