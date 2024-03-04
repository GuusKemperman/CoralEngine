#include "Precomp.h"
#include "Components/Abilities/CharacterComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::CharacterComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<CharacterComponent>{}, "CharacterComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&CharacterComponent::mTeamId, "mTeamId").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mGlobalCooldown, "mGlobalCooldown").GetProperties().Add(Props::sIsScriptableTag);
	auto& globalCooldownTimerProps = metaType.AddField(&CharacterComponent::mGlobalCooldownTimer, "mGlobalCooldownTimer").GetProperties();
	globalCooldownTimerProps.Add(Props::sIsScriptableTag);
	globalCooldownTimerProps.Add(Props::sNoInspectTag);
	metaType.AddField(&CharacterComponent::mBaseHealth, "mBaseHealth").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentHealth, "mCurrentHealth").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mBaseMovementSpeed, "mBaseMovementSpeed").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&CharacterComponent::mCurrentMovementSpeed, "mCurrentMovementSpeed").GetProperties().Add(Props::sIsScriptableTag);

	ReflectComponentType<CharacterComponent>(metaType);

	return metaType;
}
