#pragma once

#include "Assets/Core/AssetHandle.h"
#include "Meta/MetaReflect.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace CE
{
	struct DurationalEffect;
	class Prefab;
	class CharacterComponent;
	class World;
	class Weapon;

	class AbilityFunctionality
	{
	public:

		enum class Stat
		{
			Health,
			MovementSpeed,
			DealtDamageModifier,
			ReceivedDamageModifier
		};

		enum class FlatOrPercentage
		{
			Flat,
			Percentage
		};

		enum class IncreaseOrDecrease
		{
			Decrease,
			Increase
		};

		struct AbilityEffect
		{
			Stat mStat = Stat::Health;
			float mAmount{};
			FlatOrPercentage mFlatOrPercentage = FlatOrPercentage::Flat;
			IncreaseOrDecrease mIncreaseOrDecrease = IncreaseOrDecrease::Decrease;
			bool mClampToMax = true;

			bool operator==(const AbilityEffect& effectSettings) const;
			bool operator!=(const AbilityEffect& effectSettings) const;

#ifdef EDITOR
			void DisplayWidget();
#endif // EDITOR

		private:
			friend ReflectAccess;
			static MetaType Reflect();
			REFLECT_AT_START_UP(AbilityEffect);
		};

		static std::optional<float> ApplyInstantEffect(World& world, const CharacterComponent& castByCharacterData, entt::entity affectedEntity, AbilityEffect effect);
		static void ApplyDurationalEffect(World& world, const CharacterComponent& castByCharacterData, entt::entity affectedEntity, AbilityEffect effect, float duration = 0.f);
		static void RevertDurationalEffect(CharacterComponent& characterComponent, const DurationalEffect& durationalEffect);
		static void ApplyOverTimeEffect(World& world, const CharacterComponent& castByCharacterData, entt::entity affectedEntity, AbilityEffect effect, float duration = 0.f, int ticks = 1);
		static entt::entity SpawnAbilityPrefab(World& world, const Prefab& prefab, entt::entity castBy);
		static entt::entity SpawnWeaponPrefab(World& world, const Prefab& prefab, entt::entity castBy, const AssetHandle<Weapon>& weapon);

	private:
		static std::pair<float&, float&> GetStat(Stat stat, CharacterComponent& characterComponent);
		static glm::vec3 GetEffectColor(Stat stat, IncreaseOrDecrease increaseOrDecrease);

		struct PairOfOptionalAndValueHash // to use a key in std::unordered_map
		{
			template <class T1, class T2>
			std::size_t operator() (const std::pair<T1, T2>& pair) const
			{
				auto hash1 = pair.first ? std::hash<T1>{}(*pair.first) : 0; // Hash of std::optional of first value
				auto hash2 = std::hash<T2>{}(pair.second); // Hash of second value
				return hash1 ^ (hash2 << 1); // Combine hashes
			}
		};

		inline static std::unordered_map<std::pair<std::optional<Stat>, IncreaseOrDecrease>, glm::vec3, PairOfOptionalAndValueHash>
			sDefaultEffectColors = {
		{{std::nullopt, IncreaseOrDecrease::Increase}, {1.f, 1.f, 1.f}},               // Default increase - white
		{{std::nullopt, IncreaseOrDecrease::Decrease}, {0.05f, 0.05f, 0.05f}},         // Default decrease - black
		{{Stat::Health, IncreaseOrDecrease::Increase}, {0.f, 1.f, 0.f}},                    // green
		{{Stat::Health, IncreaseOrDecrease::Decrease}, {1.f, 0.f, 0.f}},                    // red
		{{Stat::MovementSpeed, IncreaseOrDecrease::Increase}, {1.f, 0.5f, 0.f}},            // orange
		{{Stat::MovementSpeed, IncreaseOrDecrease::Decrease}, {0.25f, 0.25f, 1.f}},         // blue
		{{Stat::DealtDamageModifier, IncreaseOrDecrease::Increase}, {0.75f, 0.25f, 0.75f}}, // purple
		{{Stat::ReceivedDamageModifier, IncreaseOrDecrease::Increase}, {1.f, 1.f, 0.25f}},  // yellow
		};

		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityFunctionality);
	};

	template<class Archive>
	void serialize(Archive& ar, AbilityFunctionality::AbilityEffect& v)
	{
		ar(v.mStat, v.mAmount, v.mFlatOrPercentage, v.mIncreaseOrDecrease, v.mClampToMax);
	}
}

template<>
struct Reflector<CE::AbilityFunctionality::Stat>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(Stat, CE::AbilityFunctionality::Stat);

template<>
struct CE::EnumStringPairsImpl<CE::AbilityFunctionality::Stat>
{
	static constexpr EnumStringPairs<AbilityFunctionality::Stat, 4> value = {
		EnumStringPair<AbilityFunctionality::Stat>{ AbilityFunctionality::Stat::Health, "Health" },
		{ AbilityFunctionality::Stat::MovementSpeed, "MovementSpeed" },
		{ AbilityFunctionality::Stat::DealtDamageModifier, "DealtDamageModifier" },
		{ AbilityFunctionality::Stat::ReceivedDamageModifier, "ReceivedDamageModifier" },
	};
};

template<>
struct Reflector<CE::AbilityFunctionality::FlatOrPercentage>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(FlatOrPercentage, CE::AbilityFunctionality::FlatOrPercentage);

template<>
struct CE::EnumStringPairsImpl<CE::AbilityFunctionality::FlatOrPercentage>
{
	static constexpr EnumStringPairs<AbilityFunctionality::FlatOrPercentage, 2> value = {
		EnumStringPair<AbilityFunctionality::FlatOrPercentage>{ AbilityFunctionality::FlatOrPercentage::Flat, "Flat" },
		{ AbilityFunctionality::FlatOrPercentage::Percentage, "Percentage" },
	};
};


template<>
struct Reflector<CE::AbilityFunctionality::IncreaseOrDecrease>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(IncreaseOrDecrease, CE::AbilityFunctionality::IncreaseOrDecrease);

template<>
struct CE::EnumStringPairsImpl<CE::AbilityFunctionality::IncreaseOrDecrease>
{
	static constexpr EnumStringPairs<AbilityFunctionality::IncreaseOrDecrease, 2> value = {
		EnumStringPair<AbilityFunctionality::IncreaseOrDecrease>{ AbilityFunctionality::IncreaseOrDecrease::Increase, "Increase" },
		{ AbilityFunctionality::IncreaseOrDecrease::Decrease, "Decrease" },
	};
};

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::AbilityFunctionality::AbilityEffect, var.DisplayWidget(); (void)name;)
#endif // EDITOR