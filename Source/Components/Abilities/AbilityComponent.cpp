#include "Precomp.h"
#include "Components/Abilities/AbilityComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

Engine::MetaType Engine::AbilityComponent::Reflect()
{
	MetaType metaType = MetaType{ MetaType::T<AbilityComponent>{}, "AbilityComponent" };
	metaType.GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&AbilityComponent::mDescription, "mDescription").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityComponent::mIconTextureName, "mIconTextureName").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&AbilityComponent::mTarget, "mTarget").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&AbilityComponent::mGlobalCooldown, "mGlobalCooldown").GetProperties().Add(Props::sIsScriptableTag);

	metaType.AddField(&AbilityComponent::mRequirementType, "mRequirementType").GetProperties().Add(Props::sIsScriptableTag);
	metaType.AddField(&AbilityComponent::mRequirementToUse, "mRequirementToUse").GetProperties().Add(Props::sIsScriptableTag);
	auto& requirementCounterProps = metaType.AddField(&AbilityComponent::mRequirementCounter, "mRequirementCounter").GetProperties();
	requirementCounterProps.Add(Props::sIsScriptableTag);
	requirementCounterProps.Add(Props::sNoInspectTag);

	metaType.AddField(&AbilityComponent::mCharges, "mCharges").GetProperties().Add(Props::sIsScriptableTag);
	auto& currentChargesProps = metaType.AddField(&AbilityComponent::mCurrentCharges, "mCurrentCharges").GetProperties();
	currentChargesProps.Add(Props::sIsScriptableTag);
	currentChargesProps.Add(Props::sNoInspectTag);

	auto& castByPlayerProps = metaType.AddField(&AbilityComponent::mCastByPlayer, "mCastByPlayer").GetProperties();
	castByPlayerProps.Add(Props::sIsScriptableTag);
	castByPlayerProps.Add(Props::sNoInspectTag);
	auto& hitPlayersProps = metaType.AddField(&AbilityComponent::mHitPlayers, "mHitPlayers").GetProperties();
	hitPlayersProps.Add(Props::sIsScriptableTag);
	hitPlayersProps.Add(Props::sNoInspectTag);

	ReflectComponentType<AbilityComponent>(metaType);

	return metaType;
}

Engine::MetaType Reflector<Engine::AbilityComponent::Target>::Reflect()
{
	using namespace Engine;
	using T = AbilityComponent::Target;
	MetaType type{ MetaType::T<T>{}, "Ability Target" };

	type.GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}

Engine::MetaType Reflector<Engine::AbilityComponent::RequirementType>::Reflect()
{
	using namespace Engine;
	using T = AbilityComponent::RequirementType;
	MetaType type{ MetaType::T<T>{}, "Ability RequirementType" };

	type.GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::equal_to<T>(), OperatorType::equal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	type.AddFunc(std::not_equal_to<T>(), OperatorType::inequal, MetaFunc::ExplicitParams<const T&, const T&>{}).GetProperties().Add(Props::sIsScriptableTag);
	ReflectFieldType<T>(type);

	return type;
}
