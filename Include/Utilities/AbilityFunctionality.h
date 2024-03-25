#pragma once
#include "Meta/MetaReflect.h"

namespace Engine
{
	struct DurationalEffect;
	class Prefab;
	class CharacterComponent;
	class World;

	class AbilityFunctionality
	{
	public:
		enum Stat
		{
			Health,
			MovementSpeed,
			DealtDamageModifier,
			ReceivedDamageModifier
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

		struct EffectSettings
		{
			EffectSettings()
				:
				mStat(Health),
				mAmount(0.0f),
				mFlatOrPercentage(Flat),
				mIncreaseOrDecrease(Decrease)
			{}

			EffectSettings(Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease)
				:
				mStat(stat),
				mAmount(amount),
				mFlatOrPercentage(flatOrPercentage),
				mIncreaseOrDecrease(increaseOrDecrease)
			{}

			Stat mStat = Health;
			float mAmount{};
			FlatOrPercentage mFlatOrPercentage = Flat;
			IncreaseOrDecrease mIncreaseOrDecrease = Decrease;

			bool operator==(const EffectSettings& effectSettings) const;
			bool operator!=(const EffectSettings& effectSettings) const;

		private:
			friend ReflectAccess;
			static MetaType Reflect();
			REFLECT_AT_START_UP(EffectSettings);
		};

		static std::optional<float> ApplyInstantEffect(World& world, entt::entity castByEntity, entt::entity affectedEntity, EffectSettings effect = {});
		static void ApplyDurationalEffect(World& world, entt::entity castByEntity, entt::entity affectedEntity, EffectSettings effect = {}, float duration = 0.f);
		static void RevertDurationalEffect(CharacterComponent& characterComponent, DurationalEffect& durationalEffect);
		static void ApplyOverTimeEffect(World& world, entt::entity castByEntity, entt::entity affectedEntity, EffectSettings effect = {}, float duration = 0.f, int ticks = 1);
		static void ApplyInstantEffectForOverTimeEffect(World& world, entt::entity affectedEntity, EffectSettings effect = {}, float dealtDamageModifierOfCastByCharacter = 0.f);
		static entt::entity SpawnProjectile(World& world, const Prefab& prefab, entt::entity castBy);
		static entt::entity SpawnAOE(World& world, const Prefab& prefab, entt::entity castBy); // area of attack

	private:
		static std::pair<float&, float&> GetStat(Stat stat, CharacterComponent& characterComponent);

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityFunctionality);
	};

	template<class Archive>
	void serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] const AbilityFunctionality::EffectSettings& value)
	{
		// We don't need to actually serialize it, but otherwise we get a compilation error
	}
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
	static constexpr EnumStringPairs<AbilityFunctionality::Stat, 4> value = {
		EnumStringPair<AbilityFunctionality::Stat>{ AbilityFunctionality::Stat::Health, "Health" },
		{ AbilityFunctionality::Stat::MovementSpeed, "MovementSpeed" },
		{ AbilityFunctionality::Stat::DealtDamageModifier, "DealtDamageModifier" },
		{ AbilityFunctionality::Stat::ReceivedDamageModifier, "ReceivedDamageModifier" },
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