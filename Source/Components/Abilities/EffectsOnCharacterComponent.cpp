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
