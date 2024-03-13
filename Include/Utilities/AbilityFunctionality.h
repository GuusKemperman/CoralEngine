#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

namespace Engine
{
	class Prefab;
	class CharacterComponent;
	class World;

	class AbilityFunctionality
	{
	public:
		enum Stat
		{
			Health,
			MovementSpeed
		};

		enum FlatOrPercentage
		{
			Flat,
			Percentage
		};

		enum IncreaseOrDecrease
		{
			Decrease,
			Increase
		};

		static void ApplyInstantEffect(World& world, entt::entity affectedEntity, Stat stat = Stat::Health, float amount = 0.f, FlatOrPercentage flatOrPercentage = FlatOrPercentage::Flat, IncreaseOrDecrease increaseOrDecrease = IncreaseOrDecrease::Decrease);
		static void SpawnProjectile(World& world, const Prefab& prefab, entt::entity castBy);

	private:
		static std::pair<float&, float&> GetStat(Stat stat, CharacterComponent& characterComponent);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityFunctionality);
	};
}

template<>
struct Reflector<Engine::AbilityFunctionality::Stat>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(Stat, Engine::AbilityFunctionality::Stat);

template<>
struct Engine::EnumStringPairsImpl<Engine::AbilityFunctionality::Stat>
{
	static constexpr EnumStringPairs<AbilityFunctionality::Stat, 2> value = {
		EnumStringPair<AbilityFunctionality::Stat>{ AbilityFunctionality::Stat::Health, "Health" },
		{ AbilityFunctionality::Stat::MovementSpeed, "MovementSpeed" },
	};
};

template<>
struct Reflector<Engine::AbilityFunctionality::FlatOrPercentage>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(FlatOrPercentage, Engine::AbilityFunctionality::FlatOrPercentage);

template<>
struct Engine::EnumStringPairsImpl<Engine::AbilityFunctionality::FlatOrPercentage>
{
	static constexpr EnumStringPairs<AbilityFunctionality::FlatOrPercentage, 2> value = {
		EnumStringPair<AbilityFunctionality::FlatOrPercentage>{ AbilityFunctionality::FlatOrPercentage::Flat, "Flat" },
		{ AbilityFunctionality::FlatOrPercentage::Percentage, "Percentage" },
	};
};


template<>
struct Reflector<Engine::AbilityFunctionality::IncreaseOrDecrease>
{
	static Engine::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(IncreaseOrDecrease, Engine::AbilityFunctionality::IncreaseOrDecrease);

template<>
struct Engine::EnumStringPairsImpl<Engine::AbilityFunctionality::IncreaseOrDecrease>
{
	static constexpr EnumStringPairs<AbilityFunctionality::IncreaseOrDecrease, 2> value = {
		EnumStringPair<AbilityFunctionality::IncreaseOrDecrease>{ AbilityFunctionality::IncreaseOrDecrease::Increase, "Increase" },
		{ AbilityFunctionality::IncreaseOrDecrease::Decrease, "Decrease" },
	};
};