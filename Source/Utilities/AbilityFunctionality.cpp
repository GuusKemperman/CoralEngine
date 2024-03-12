#include "Precomp.h"
#include "Utilities/AbilityFunctionality.h"

#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "World/Registry.h"
#include "World/World.h"

void Engine::AbilityFunctionality::ApplyInstantEffect(World& world, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease)
{
	auto& reg = world.GetRegistry();
	auto& characterComponent = reg.Get<CharacterComponent>(affectedEntity);
	auto [base, current] = GetStat(stat, characterComponent);

	if (flatOrPercentage == FlatOrPercentage::Percentage)
	{
		amount = amount * 0.01f;
	}

	// in the future apply damage modifier

	if (increaseOrDecrease == IncreaseOrDecrease::Decrease)
	{
		amount = -amount;
	}

	// apply
	current += amount;
	current = std::max(current, 0.0f);
	//if (ability.clampToMax)
		current = std::min(current, base);
}

std::pair<float&, float&> Engine::AbilityFunctionality::GetStat(Stat stat, CharacterComponent& characterComponent)
{
	switch(stat)
	{
	case Health:
		return { characterComponent.mBaseHealth, characterComponent.mCurrentHealth };
	case MovementSpeed:
		return { characterComponent.mBaseMovementSpeed, characterComponent.mCurrentMovementSpeed };
	}

	// because of the stupid warning "not all control paths return a value"
	return { characterComponent.mBaseHealth, characterComponent.mCurrentHealth };
}

Engine::MetaType Engine::AbilityFunctionality::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityFunctionality>{}, "AbilityFunctionality" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	//metaType.AddFunc([](entt::entity castBy, CharacterComponent& characterData)
	//	{
	//		World* world = World::TryGetWorldAtTopOfStack();
	//		ASSERT(world != nullptr);

	//		AbilitySystem::ActivateAbility(*world, castBy, characterData, ability);

	//	}, "ActivateAbility", MetaFunc::ExplicitParams<entt::entity, CharacterComponent&>{}).GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

	return metaType;
}
