#include "Precomp.h"
#include "Components/WavesComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Reflect/ReflectFieldType.h"

bool Game::EnemyTypeAndCount::operator==(const EnemyTypeAndCount& other) const
{
	return (mEnemy == other.mEnemy && mAmount == other.mAmount);
}

bool Game::EnemyTypeAndCount::operator!=(const EnemyTypeAndCount& other) const
{
	return !(*this == other);
}

#ifdef EDITOR
void Game::EnemyTypeAndCount::DisplayWidget()
{
	ShowInspectUI("mEnemy", mEnemy);
	CE::ShowInspectUI("mAmount", mAmount);
}
#endif // EDITOR

CE::MetaType Game::EnemyTypeAndCount::Reflect()
{
	auto metaType = CE::MetaType{CE::MetaType::T<EnemyTypeAndCount>{}, "EnemyTypeAndCount"};
	metaType.GetProperties().Add(CE::Props::sIsScriptableTag).Add(CE::Props::sIsScriptOwnableTag);

	metaType.AddField(&EnemyTypeAndCount::mEnemy, "mEnemy").GetProperties().Add(CE::Props::sIsScriptableTag);
	metaType.AddField(&EnemyTypeAndCount::mAmount, "mAmount").GetProperties().Add(CE::Props::sIsScriptableTag);

	return metaType;
}

CE::MetaType Game::WavesComponent::Reflect()
{
	auto type = CE::MetaType{CE::MetaType::T<WavesComponent>{}, "WavesComponent"};
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);

	type.AddField(&WavesComponent::mWaves, "mWaves").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<WavesComponent>(type);

	return type;
}
