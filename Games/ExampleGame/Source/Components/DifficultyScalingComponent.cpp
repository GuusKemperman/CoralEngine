#include "Precomp.h"
#include "Components/DifficultyScalingComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType Game::DifficultyScalingComponent::Reflect()
{
	CE::MetaType type = CE::MetaType{ CE::MetaType::T<DifficultyScalingComponent>{}, "DifficultyScalingComponent" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mDoesRepeat, "mDoesRepeat").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mScaleLength, "mScaleLength").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mScaleHPOverTime, "mScaleHPOverTime");
	type.AddField(&DifficultyScalingComponent::mMinHPMultiplier, "mMinHPMultiplier").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mMaxHPMultiplier, "mMaxHPMultiplier").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mCurrentHPMultiplier, "mCurrentHPMultiplier").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag);
	type.AddField(&DifficultyScalingComponent::mScaleDamageOverTime, "mScaleDamageOverTime");
	type.AddField(&DifficultyScalingComponent::mMinDamageMultiplier, "mMinDamageMultiplier").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mMaxDamageMultiplier, "mMaxDamageMultiplier").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mCurrentDamageMultiplier, "mCurrentDamageMultiplier").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag);

	CE::ReflectComponentType<DifficultyScalingComponent>(type);
	return type;
}