#include "Precomp.h"
#include "Systems/AbilitySystem.h"

#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"

void Engine::AbilitySystem::Update(World& world, float dt)
{
    Registry& reg = world.GetRegistry();
    auto view = reg.View<CharacterComponent>();
    for (auto [entity, characterData] : view.each())
    {
        LOG(LogPhysics, Message, "Character entity: {}, DeltaTime: {}", entt::to_integral(entity), dt)
    }
}

Engine::MetaType Engine::AbilitySystem::Reflect()
{
	return MetaType{ MetaType::T<AbilitySystem>{}, "AbilitySystem", MetaType::Base<System>{} };
}
