#include "Precomp.h"
#include "Components/Abilities/CharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::CharacterComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<CharacterComponent>{}, "CharacterComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);

	metaType.AddField(&CharacterComponent::mTeamId, "mTeamId").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mGlobalCooldown, "mGlobalCooldown").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mGlobalCooldownTimer, "mGlobalCooldownTimer").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&CharacterComponent::mBaseHealth, "mBaseHealth").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentHealth, "mCurrentHealth").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&CharacterComponent::mBaseMovementSpeed, "mBaseMovementSpeed").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentMovementSpeed, "mCurrentMovementSpeed").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&CharacterComponent::mBaseDealtDamageModifier, "mBaseDealtDamageModifier").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentDealtDamageModifier, "mCurrentDealtDamageModifier").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);
	metaType.AddField(&CharacterComponent::mBaseReceivedDamageModifier, "mBaseReceivedDamageModifier").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentReceivedDamageModifier, "mCurrentReceivedDamageModifier").GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsEditorReadOnlyTag);

	ReflectComponentType<CharacterComponent>(metaType);

	return metaType;
}
