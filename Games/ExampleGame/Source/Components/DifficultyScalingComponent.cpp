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
	type.AddField(&DifficultyScalingComponent::mIsRepeating, "mIsRepeating").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mLoopsElapsed, "mLoopsElapsed").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag).Add(CE::Props::sIsScriptReadOnlyTag);
	type.AddField(&DifficultyScalingComponent::mScaleTime, "mScaleTime").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mScaleHPOverTime, "mScaleHPOverTime");
	type.AddField(&DifficultyScalingComponent::mMinHealthMultiplier, "mMinHealthMultiplier").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mMaxHealthMultiplier, "mMaxHealthMultiplier").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mCurrentHealthMultiplier, "mCurrentHealthMultiplier").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag).Add(CE::Props::sIsScriptReadOnlyTag);
	type.AddField(&DifficultyScalingComponent::mScaleDamageOverTime, "mScaleDamageOverTime");
	type.AddField(&DifficultyScalingComponent::mMinDamageMultiplier, "mMinDamageMultiplier").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mMaxDamageMultiplier, "mMaxDamageMultiplier").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&DifficultyScalingComponent::mCurrentDamageMultiplier, "mCurrentDamageMultiplier").GetProperties().Add(CE::Props::sIsEditorReadOnlyTag).Add(CE::Props::sIsScriptReadOnlyTag);

	CE::ReflectComponentType<DifficultyScalingComponent>(type);
	return type;
}