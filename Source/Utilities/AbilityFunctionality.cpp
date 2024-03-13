#include "Precomp.h"
#include "Utilities/AbilityFunctionality.h"

#include "Components/Abilities/CharacterComponent.h"
#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "World/Registry.h"
#include "World/World.h"

Engine::MetaType Engine::AbilityFunctionality::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityFunctionality>{}, "AbilityFunctionality" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddFunc([](entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease)
		{
			World* world = World::TryGetWorldAtTopOfStack();
			ASSERT(world != nullptr);

			ApplyInstantEffect(*world, affectedEntity, stat, amount, flatOrPercentage, increaseOrDecrease);

		}, "ApplyInstantEffect", MetaFunc::ExplicitParams<
		entt::entity, Stat, float, FlatOrPercentage, IncreaseOrDecrease>{}, "ApplyToEntity", "Stat", "Amount").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, false);

		return metaType;
}

void Engine::AbilityFunctionality::ApplyInstantEffect(World& world, entt::entity affectedEntity, Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease)
{
	auto& reg = world.GetRegistry();
	auto& characterComponent = reg.Get<CharacterComponent>(affectedEntity);
	auto [base, current] = GetStat(stat, characterComponent);

	if (flatOrPercentage == FlatOrPercentage::Percentage)
	{
		amount = amount * 0.01f * base;
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

Engine::MetaType Reflector<Engine::AbilityFunctionality::Stat>::Reflect()
{
	using namespace Engine;
	using T = AbilityFunctionality::Stat;
	MetaType type{ MetaType::T<T>{}, "Stat" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

Engine::MetaType Reflector<Engine::AbilityFunctionality::FlatOrPercentage>::Reflect()
{
	using namespace Engine;
	using T = AbilityFunctionality::FlatOrPercentage;
	MetaType type{ MetaType::T<T>{}, "FlatOrPercentage" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

Engine::MetaType Reflector<Engine::AbilityFunctionality::IncreaseOrDecrease>::Reflect()
{
	using namespace Engine;
	using T = AbilityFunctionality::IncreaseOrDecrease;
	MetaType type{ MetaType::T<T>{}, "IncreaseOrDecrease" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}
