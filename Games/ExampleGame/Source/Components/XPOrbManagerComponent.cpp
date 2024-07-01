#include "Precomp.h"
#include "Components/XPOrbManagerComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType Game::XPOrbManagerComponent::Reflect()
{
	CE::MetaType type{ CE::MetaType::T<XPOrbManagerComponent>{}, "XPOrbManagerComponent" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);

	type.AddField(&XPOrbManagerComponent::mXPValue, "XPValue").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbManagerComponent::mChaseRange, "ChaseRange").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbManagerComponent::mChaseSpeed, "ChaseSpeed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbManagerComponent::mRotationSpeed, "RotationSpeed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbManagerComponent::mHoverSpeed, "Hover Speed").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbManagerComponent::mHoverHeight, "Hover Height").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbManagerComponent::mMaxTimeAlive, "mMaxTimeAlive").GetProperties().Add(CE::Props::sIsScriptableTag);
	type.AddField(&XPOrbManagerComponent::mMaxAlive, "mMaxAlive").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<XPOrbManagerComponent>(type);
	return type;
}
