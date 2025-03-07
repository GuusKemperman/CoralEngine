#include "Precomp.h"
#include "Components/Abilities/EffectsOnCharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Math.h"

CE::MetaType CE::EffectsOnCharacterComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<EffectsOnCharacterComponent>{}, "EffectsOnCharacterComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&EffectsOnCharacterComponent::mDurationalEffects, "mDurationalEffects").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoSerializeTag);
	metaType.AddField(&EffectsOnCharacterComponent::mOverTimeEffects, "mOverTimeEffects").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoSerializeTag);
	metaType.AddField(&EffectsOnCharacterComponent::mVisualEffects, "mVisualEffects").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoSerializeTag);

	ReflectComponentType<EffectsOnCharacterComponent>(metaType);

	return metaType;
}

bool CE::DurationalEffect::operator==(const DurationalEffect& other) const
{
	return Math::AreFloatsEqual(mDuration, other.mDuration) &&
		Math::AreFloatsEqual(mDurationTimer, other.mDurationTimer) &&
		mStatAffected == other.mStatAffected &&
		Math::AreFloatsEqual(mAmount, other.mAmount);
}

bool CE::DurationalEffect::operator!=(const DurationalEffect& other) const
{
	return !(*this == other);
}

#ifdef EDITOR
void CE::DurationalEffect::DisplayWidget()
{
	ShowInspectUIReadOnly("mDuration", mDuration);
	ShowInspectUIReadOnly("mDurationTimer", mDurationTimer);
	ShowInspectUIReadOnly("mStatAffected", mStatAffected);
	ShowInspectUIReadOnly("mAmount", mAmount);
}
#endif // EDITOR

CE::MetaType CE::DurationalEffect::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DurationalEffect>{}, "DurationalEffect" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&DurationalEffect::mDuration, "mDuration").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&DurationalEffect::mDurationTimer, "mDurationTimer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&DurationalEffect::mStatAffected, "mStatAffected").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&DurationalEffect::mAmount, "mAmount").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);

	return metaType;
}

bool CE::OverTimeEffect::operator==(const OverTimeEffect& other) const
{
	return Math::AreFloatsEqual(mTickDuration, other.mTickDuration) &&
		Math::AreFloatsEqual(mDurationTimer, other.mDurationTimer) &&
		mNumberOfTicks == other.mNumberOfTicks &&
		mTicksCounter == other.mTicksCounter &&
		mEffectSettings == other.mEffectSettings/* && 
		mCastByCharacterData == other.mCastByCharacterData*/;
}

bool CE::OverTimeEffect::operator!=(const OverTimeEffect& other) const
{
	return !(*this == other);
}

#ifdef EDITOR
void CE::OverTimeEffect::DisplayWidget()
{
	ShowInspectUIReadOnly("mTickDuration", mTickDuration);
	ShowInspectUIReadOnly("mDurationTimer", mDurationTimer);
	ShowInspectUIReadOnly("mNumberOfTicks", mNumberOfTicks);
	ShowInspectUIReadOnly("mTicksCounter", mTicksCounter);
	ShowInspectUIReadOnly("mStatAffected", mEffectSettings.mStat);
	ShowInspectUIReadOnly("mAmount", mEffectSettings.mAmount);
	ShowInspectUIReadOnly("mFlatOrPercentage", mEffectSettings.mFlatOrPercentage);
	ShowInspectUIReadOnly("mIncreaseOrDecrease", mEffectSettings.mIncreaseOrDecrease);
	ShowInspectUIReadOnly("CastByCharacterTeamID", mCastByCharacterData.mTeamId);
}
#endif // EDITOR

CE::MetaType CE::OverTimeEffect::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<OverTimeEffect>{}, "OverTimeEffect" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&OverTimeEffect::mTickDuration, "mTickDuration").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mDurationTimer, "mDurationTimer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mNumberOfTicks, "mNumberOfTicks").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mTicksCounter, "mTicksCounter").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mEffectSettings, "mEffectSettings").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mCastByCharacterData, "mCastByCharacterData").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);

	return metaType;
}

bool CE::VisualEffect::operator==(const VisualEffect& other) const
{
	return Math::AreFloatsEqual(mColor.x, other.mColor.x) && 
		Math::AreFloatsEqual(mColor.y, other.mColor.y) &&
		Math::AreFloatsEqual(mColor.z, other.mColor.z) &&
		Math::AreFloatsEqual(mDuration, other.mDuration) &&
		Math::AreFloatsEqual(mDurationTimer, other.mDurationTimer);
}

bool CE::VisualEffect::operator!=(const VisualEffect& other) const
{
	return !(*this == other);
}

#ifdef EDITOR
void CE::VisualEffect::DisplayWidget()
{
	ShowInspectUIReadOnly("mColor", mColor);
	ShowInspectUIReadOnly("mDuration", mDuration);
	ShowInspectUIReadOnly("mDurationTimer", mDurationTimer);
}
#endif // EDITOR

CE::MetaType CE::VisualEffect::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<VisualEffect>{}, "VisualEffect" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&VisualEffect::mColor, "mColor").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&VisualEffect::mDuration, "mDuration").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&VisualEffect::mDurationTimer, "mDurationTimer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	
	return metaType;
}
