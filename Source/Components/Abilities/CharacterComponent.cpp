#include "Precomp.h"
#include "Components/Abilities/CharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Math.h"
#include "Utilities/Reflect/ReflectComponentType.h"

bool CE::CharacterComponent::operator==(const CharacterComponent& other) const
{
	return mTeamId == other.mTeamId &&
		Math::AreFloatsEqual(mGlobalCooldown, other.mGlobalCooldown) &&
		Math::AreFloatsEqual(mGlobalCooldownTimer, other.mGlobalCooldownTimer) &&
		Math::AreFloatsEqual(mBaseHealth, other.mBaseHealth) &&
		Math::AreFloatsEqual(mCurrentHealth, other.mCurrentHealth) &&
		Math::AreFloatsEqual(mBaseMovementSpeed, other.mBaseMovementSpeed) &&
		Math::AreFloatsEqual(mCurrentMovementSpeed, other.mCurrentMovementSpeed) &&
		Math::AreFloatsEqual(mBaseDealtDamageModifier, other.mBaseDealtDamageModifier) &&
		Math::AreFloatsEqual(mCurrentDealtDamageModifier, other.mCurrentDealtDamageModifier) &&
		Math::AreFloatsEqual(mBaseReceivedDamageModifier, other.mBaseReceivedDamageModifier) &&
		Math::AreFloatsEqual(mCurrentReceivedDamageModifier, other.mCurrentReceivedDamageModifier);
}

bool CE::CharacterComponent::operator!=(const CharacterComponent& other) const
{
	return !(*this == other);
}

CE::MetaType CE::CharacterComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<CharacterComponent>{}, "CharacterComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&CharacterComponent::mTeamId, "mTeamId").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mGlobalCooldown, "mGlobalCooldown").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mGlobalCooldownTimer, "mGlobalCooldownTimer").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mBaseHealth, "mBaseHealth").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentHealth, "mCurrentHealth").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mBaseMovementSpeed, "mBaseMovementSpeed").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentMovementSpeed, "mCurrentMovementSpeed").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mBaseDealtDamageModifier, "mBaseDealtDamageModifier").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentDealtDamageModifier, "mCurrentDealtDamageModifier").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mBaseReceivedDamageModifier, "mBaseReceivedDamageModifier").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentReceivedDamageModifier, "mCurrentReceivedDamageModifier").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<CharacterComponent>(metaType);

	return metaType;
}
