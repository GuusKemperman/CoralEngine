#pragma once
#include "Meta/MetaReflect.h"
#include "Utilities/Imgui/ImguiInspect.h"

namespace CE
{
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
		bool mClampToMax{};
		float mCritChance{};         // Percentage from 0 to 100
		float mCritIncrease = 100.f; // Percentage from 0 to 100

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

	class AbilityEffectsComponent
	{
	public:
		std::vector<AbilityEffect> mEffects{};

	private:
		friend ReflectAccess;
		static MetaType Reflect();
		REFLECT_AT_START_UP(AbilityEffectsComponent);
	};

	template<class Archive>
	void load(Archive& ar, AbilityEffect& v)
	{
		ar(v.mStat, v.mAmount, v.mFlatOrPercentage, v.mIncreaseOrDecrease, v.mClampToMax, v.mCritChance, v.mCritIncrease);
	}

	template<class Archive>
	void save(Archive& ar, const AbilityEffect& v)
	{
		ar(v.mStat, v.mAmount, v.mFlatOrPercentage, v.mIncreaseOrDecrease, v.mClampToMax, v.mCritChance, v.mCritIncrease);
	}
}

template<>
struct Reflector<CE::Stat>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(Stat, CE::Stat);

template<>
struct CE::EnumStringPairsImpl<CE::Stat>
{
	static constexpr EnumStringPairs<Stat, 4> value = {
		EnumStringPair<Stat>{ Stat::Health, "Health" },
		{ Stat::MovementSpeed, "MovementSpeed" },
		{ Stat::DealtDamageModifier, "DealtDamageModifier" },
		{ Stat::ReceivedDamageModifier, "ReceivedDamageModifier" },
	};
};

template<>
struct Reflector<CE::FlatOrPercentage>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(FlatOrPercentage, CE::FlatOrPercentage);

template<>
struct CE::EnumStringPairsImpl<CE::FlatOrPercentage>
{
	static constexpr EnumStringPairs<FlatOrPercentage, 2> value = {
		EnumStringPair<FlatOrPercentage>{ FlatOrPercentage::Flat, "Flat" },
		{ FlatOrPercentage::Percentage, "Percentage" },
	};
};

template<>
struct Reflector<CE::IncreaseOrDecrease>
{
	static CE::MetaType Reflect();
	static constexpr bool sIsSpecialized = true;
}; REFLECT_AT_START_UP(IncreaseOrDecrease, CE::IncreaseOrDecrease);

template<>
struct CE::EnumStringPairsImpl<CE::IncreaseOrDecrease>
{
	static constexpr EnumStringPairs<IncreaseOrDecrease, 2> value = {
		EnumStringPair<IncreaseOrDecrease>{ IncreaseOrDecrease::Increase, "Increase" },
		{ IncreaseOrDecrease::Decrease, "Decrease" },
	};
};

#ifdef EDITOR
IMGUI_AUTO_DEFINE_INLINE(template<>, CE::AbilityEffect, var.DisplayWidget(); (void)name;)
#endif // EDITOR