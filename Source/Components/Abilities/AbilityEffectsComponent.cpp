#include "Precomp.h"
#include "Components/Abilities/AbilityEffectsComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Math.h"

CE::MetaType Reflector<CE::Stat>::Reflect()
{
	using namespace CE;
	using T = Stat;
	MetaType type{ MetaType::T<T>{}, "Stat" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::FlatOrPercentage>::Reflect()
{
	using namespace CE;
	using T = FlatOrPercentage;
	MetaType type{ MetaType::T<T>{}, "FlatOrPercentage" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

CE::MetaType Reflector<CE::IncreaseOrDecrease>::Reflect()
{
	using namespace CE;
	using T = IncreaseOrDecrease;
	MetaType type{ MetaType::T<T>{}, "IncreaseOrDecrease" };

	type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

bool CE::AbilityEffect::operator==(const AbilityEffect& other) const
{
	return mStat == other.mStat &&
		Math::AreFloatsEqual(mAmount, other.mAmount) &&
		mFlatOrPercentage == other.mFlatOrPercentage &&
		mIncreaseOrDecrease == other.mIncreaseOrDecrease &&
		mClampToMax == other.mClampToMax &&
		Math::AreFloatsEqual(mCritChance, other.mCritChance) &&
		Math::AreFloatsEqual(mCritIncrease, other.mCritIncrease);
}

bool CE::AbilityEffect::operator!=(const AbilityEffect& other) const
{
	return !(*this == other);
}

#ifdef EDITOR
void CE::AbilityEffect::DisplayWidget()
{
	const MetaType& weapon = MetaManager::Get().GetType<AbilityEffect>();
	MetaAny ref{ *this };
	for (const MetaField& field : weapon.EachField())
	{
		MetaAny refToMember = field.MakeRef(ref);
		ShowInspectUI(std::string{ field.GetName() }, refToMember);
	}
}
#endif // EDITOR

CE::MetaType CE::AbilityEffect::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityEffect>{}, "AbilityEffect" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&AbilityEffect::mStat, "mStat").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityEffect::mAmount, "mAmount").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityEffect::mFlatOrPercentage, "mFlatOrPercentage").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityEffect::mIncreaseOrDecrease, "mIncreaseOrDecrease").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityEffect::mClampToMax, "mClampToMax").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityEffect::mCritChance, "mCritChance").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityEffect::mCritIncrease, "mCritIncrease").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddFunc([](const Stat stat, float amount, FlatOrPercentage flatOrPercentage, IncreaseOrDecrease increaseOrDecrease, bool clampToMax, float critChance, float critIncrease) -> AbilityEffect
		{
			return AbilityEffect{ stat, amount, flatOrPercentage, increaseOrDecrease, clampToMax, critChance, critIncrease };

		}, "MakeAbilityEffect", MetaFunc::ExplicitParams<Stat, float, FlatOrPercentage, IncreaseOrDecrease, bool, float, float>{}, "Stat", "Amount", "FlatOrPercentage", "IncreaseOrDecrease", "ClampToMax", "CritChance", "CritIncrease").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	metaType.AddFunc([](const AbilityEffect abilityEffect, float newAmount) -> AbilityEffect
		{
			return AbilityEffect{ abilityEffect.mStat, newAmount, abilityEffect.mFlatOrPercentage, abilityEffect.mIncreaseOrDecrease, abilityEffect.mClampToMax, abilityEffect.mCritChance, abilityEffect.mCritIncrease };

		}, "GetAbilityEffectWithAmountChanged", MetaFunc::ExplicitParams<const AbilityEffect, float>{}, "AbilityEffect", "NewAmount").GetProperties().Add(Props::sIsScriptableTag).Set(Props::sIsScriptPure, true);

	ReflectFieldType<AbilityEffect>(metaType);
	return metaType;
}

CE::MetaType CE::AbilityEffectsComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityEffectsComponent>{}, "AbilityEffectsComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&AbilityEffectsComponent::mEffects, "mEffects").GetProperties().Add(Props::sIsScriptableTag);
	
	ReflectComponentType<AbilityEffectsComponent>(metaType);
	return metaType;
}
