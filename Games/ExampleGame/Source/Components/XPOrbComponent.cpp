#include "Precomp.h"
#include "Components/XPOrbComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"
#include "Utilities/Reflect/ReflectFieldType.h"


CE::MetaType Game::XPOrbComponent::Reflect()
{
	CE::MetaType type{ CE::MetaType::T<XPOrbComponent>{}, "XPOrbComponent" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);
	props.Set(CE::Props::sOldNames, "XPOrbScript");

	type.AddField(&XPOrbComponent::mXPValue, "XPValue").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbComponent::mChaseRange, "ChaseRange").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbComponent::mChaseSpeed, "ChaseSpeed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbComponent::mRotationSpeed, "RotationSpeed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbComponent::mHoverTime, "Hover Time").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbComponent::mHoverSpeed, "Hover Speed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbComponent::mHoverHeight, "Hover Height").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbComponent::mSecondsRemainingUntilDespawn, "SecondsRemainingUntilDespawn").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<XPOrbComponent>(type);

	return type;
}
