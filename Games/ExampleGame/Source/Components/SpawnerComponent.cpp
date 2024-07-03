#include "Precomp.h"
#include "Components/SpawnerComponent.h"

#include "Meta/MetaType.h"
#include "Meta/ReflectedTypes/STD/ReflectOptional.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Assets/Prefabs/Prefab.h"
#include "Utilities/Reflect/ReflectComponentType.h"


bool Game::SpawnerComponent::Wave::EnemyType::operator!=(const EnemyType& enemy) const
{
	return enemy.mPrefab != mPrefab
		|| enemy.mMaxAmountAlive != mMaxAmountAlive
		|| enemy.mAmountToSpawnAtStartOfWave != mAmountToSpawnAtStartOfWave
		|| enemy.mSpawnChance != mSpawnChance;
}

bool Game::SpawnerComponent::Wave::EnemyType::operator==(const EnemyType& enemy) const
{
	return !(enemy != *this);
}

#ifdef EDITOR
void Game::SpawnerComponent::Wave::EnemyType::DisplayWidget(const std::string& name)
{
	ImGui::TextUnformatted(name.c_str(), name.c_str() + name.size());
	ImGui::PushID(name.c_str(), name.c_str() + name.size());
	ImGui::Indent();

	CE::ShowInspectUI("mPrefab", mPrefab);
	CE::ShowInspectUI("mSpawnChance", mSpawnChance);
	CE::ShowInspectUI("mMaxAmountAlive", mMaxAmountAlive);
	CE::ShowInspectUI("mAmountToSpawnAtStartOfWave", mAmountToSpawnAtStartOfWave);

	ImGui::Unindent();
	ImGui::PopID();
}
#endif // EDITOR

CE::MetaType Game::SpawnerComponent::Wave::EnemyType::Reflect()
{
	CE::MetaType type = CE::MetaType{ CE::MetaType::T<EnemyType>{}, "EnemyType" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsScriptOwnableTag);

	type.AddField(&EnemyType::mPrefab, "mPrefab").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&EnemyType::mSpawnChance, "mSpawnChance").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&EnemyType::mMaxAmountAlive, "mMaxAmountAlive").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&EnemyType::mAmountToSpawnAtStartOfWave, "mAmountToSpawnAtStartOfWave").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectFieldType<EnemyType>(type);
	return type;
}

bool Game::SpawnerComponent::Wave::operator!=(const Wave& wave) const
{
	return wave.mAmountToSpawnPerSecond != mAmountToSpawnPerSecond
		|| wave.mAmountToSpawnPerSecondWhenBelowMinimum != mAmountToSpawnPerSecondWhenBelowMinimum
		|| wave.mEnemies != mEnemies
		|| wave.mDuration != mDuration
		|| wave.mDesiredMinimumNumberOfEnemies != mDesiredMinimumNumberOfEnemies
		|| wave.mOnWaveFinishedAddComponent != mOnWaveFinishedAddComponent;
}

bool Game::SpawnerComponent::Wave::operator==(const Wave& wave) const
{
	return !(wave != *this);
}

#ifdef EDITOR
void Game::SpawnerComponent::Wave::DisplayWidget(const std::string& name)
{
	ImGui::TextUnformatted(name.c_str(), name.c_str() + name.size());
	ImGui::PushID(name.c_str(), name.c_str() + name.size());
	ImGui::Indent();

	CE::ShowInspectUI("mDuration", mDuration);
	CE::ShowInspectUI("mAmountToSpawnPerSecond", mAmountToSpawnPerSecond);
	CE::ShowInspectUI("mAmountToSpawnPerSecondWhenBelowMinimum", mAmountToSpawnPerSecondWhenBelowMinimum);
	CE::ShowInspectUI("mDesiredMinimumNumberOfEnemies", mDesiredMinimumNumberOfEnemies);
	CE::ShowInspectUI("mEnemies", mEnemies);
	CE::ShowInspectUI("mOnWaveFinishedAddComponent", mOnWaveFinishedAddComponent);

	ImGui::Unindent();
	ImGui::PopID();
}
#endif // EDITOR

CE::MetaType Game::SpawnerComponent::Wave::Reflect()
{
	CE::MetaType type = CE::MetaType{ CE::MetaType::T<Wave>{}, "Wave" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsScriptOwnableTag);

	type.AddField(&Wave::mDuration, "mDuration").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Wave::mAmountToSpawnPerSecond, "mAmountToSpawnPerSecond").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Wave::mAmountToSpawnPerSecondWhenBelowMinimum, "mAmountToSpawnPerSecondWhenBelowMinimum").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Wave::mEnemies, "mEnemies").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&Wave::mOnWaveFinishedAddComponent, "mOnWaveFinishedAddComponent").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectFieldType<Wave>(type);
	return type;
}

CE::MetaType Game::SpawnerComponent::Reflect()
{
	CE::MetaType type = CE::MetaType{CE::MetaType::T<SpawnerComponent>{}, "SpawnerComponent"};
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);

	type.AddField(&SpawnerComponent::mMinSpawnRange, "mMinSpawnRange").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mMaxEnemyDistance, "mMaxEnemyDistance").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mShouldSpawnInGroups, "mShouldSpawnInGroups").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mMinRandomScale, "mMinRandomScale").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mMaxRandomScale, "mMaxRandomScale").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&SpawnerComponent::mCurrentWaveIndex, "mCurrentWaveIndex").GetProperties().Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsEditorReadOnlyTag);
	type.AddField(&SpawnerComponent::mWaves, "mWaves").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<SpawnerComponent>(type);
	return type;
}
