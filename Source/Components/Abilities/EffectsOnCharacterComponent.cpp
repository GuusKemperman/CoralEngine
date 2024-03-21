#include "Precomp.h"
#include "Components/Abilities/EffectsOnCharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Math.h"

Engine::MetaType Engine::EffectsOnCharacterComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<EffectsOnCharacterComponent>{}, "EffectsOnCharacterComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&EffectsOnCharacterComponent::mDurationalEffects, "mDurationalEffects").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoSerializeTag);
	metaType.AddField(&EffectsOnCharacterComponent::mOverTimeEffects, "mOverTimeEffects").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoSerializeTag);

	ReflectComponentType<EffectsOnCharacterComponent>(metaType);

	return metaType;
}

bool Engine::DurationalEffect::operator==(const DurationalEffect& other) const
{
	return Math::AreFloatsEqual(mDuration, other.mDuration) &&
		Math::AreFloatsEqual(mDurationTimer, other.mDurationTimer) &&
		mStatAffected == other.mStatAffected &&
		Math::AreFloatsEqual(mAmount, other.mAmount);
}

bool Engine::DurationalEffect::operator!=(const DurationalEffect& other) const
{
	return !(*this == other);
}

#ifdef EDITOR
void Engine::DurationalEffect::DisplayWidget()
{
	ImGui::TextWrapped("mDuration: %f", mDuration);
	ImGui::TextWrapped("mDurationTimer: %f", mDurationTimer);
	ImGui::TextWrapped("mAmount: %f", mAmount);
}
#endif // EDITOR

Engine::MetaType Engine::DurationalEffect::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<DurationalEffect>{}, "DurationalEffect" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&DurationalEffect::mDuration, "mDuration").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&DurationalEffect::mDurationTimer, "mDurationTimer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&DurationalEffect::mStatAffected, "mStatAffected").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&DurationalEffect::mAmount, "mAmount").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);

	return metaType;
}

bool Engine::OverTimeEffect::operator==(const OverTimeEffect& other) const
{
	return Math::AreFloatsEqual(mDuration, other.mDuration) &&
		Math::AreFloatsEqual(mDurationTimer, other.mDurationTimer) &&
		mTicks == other.mTicks &&
		mTicksCounter == other.mTicksCounter &&
		mEffectSettings == other.mEffectSettings &&
		Math::AreFloatsEqual(mDealtDamageModifierOfCastByCharacter, 
			other.mDealtDamageModifierOfCastByCharacter);
}

bool Engine::OverTimeEffect::operator!=(const OverTimeEffect& other) const
{
	return !(*this == other);
}

#ifdef EDITOR
void Engine::OverTimeEffect::DisplayWidget()
{
	ImGui::TextWrapped("mDuration: %f", mDuration);
	ImGui::TextWrapped("mDurationTimer: %f", mDurationTimer);
	ImGui::TextWrapped("mTicks: %d", mTicks);
	ImGui::TextWrapped("mTicksCounter: %d", mTicksCounter);
	ImGui::TextWrapped("Effect");
}
#endif // EDITOR

Engine::MetaType Engine::OverTimeEffect::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<OverTimeEffect>{}, "OverTimeEffect" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&OverTimeEffect::mDuration, "mDuration").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&OverTimeEffect::mDurationTimer, "mDurationTimer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mTicks, "mTicks").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mTicksCounter, "mTicksCounter").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mEffectSettings, "mEffectSettings").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);
	metaType.AddField(&OverTimeEffect::mDealtDamageModifierOfCastByCharacter, "mDealtDamageModifierOfCastByCharacter").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sNoInspectTag);

	return metaType;
}
