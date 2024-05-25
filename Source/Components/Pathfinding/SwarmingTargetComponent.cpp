#include "Precomp.h"
#include "Components/Pathfinding/SwarmingTargetComponent.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::SwarmingTargetComponent::Reflect()
{
	MetaType type{ MetaType::T<SwarmingTargetComponent>{}, "SwarmingTargetComponent" };

	type.AddField(&SwarmingTargetComponent::mDesiredRadius, "mRadius");
	type.AddField(&SwarmingTargetComponent::mSpacing, "mSpacing");

	ReflectComponentType<SwarmingTargetComponent>(type);
	return type;
}
