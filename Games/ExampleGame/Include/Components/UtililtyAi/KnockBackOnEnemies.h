#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace CE
{
	class World;
}

class KnockBackOnEnemies
{
public:
	static void OnAbilityHit(CE::World& world, const entt::entity caster, entt::entity target, entt::entity projectile);
private:
	friend CE::ReflectAccess;
	static CE::MetaType Reflect();
	REFLECT_AT_START_UP(KnockBackOnEnemies);
};
