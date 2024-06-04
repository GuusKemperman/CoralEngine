#include "Precomp.h"
#include "Components/Pathfinding/SwarmingTargetComponent.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType CE::SwarmingTargetComponent::Reflect()
{
	MetaType type{ MetaType::T<SwarmingTargetComponent>{}, "SwarmingTargetComponent" };

	type.AddField(&SwarmingTargetComponent::mDesiredRadius, "mRadius");
	type.AddField(&SwarmingTargetComponent::mNumberOfSmoothingSteps, "mNumberOfSmoothingSteps");

	ReflectComponentType<SwarmingTargetComponent>(type);
	return type;
}
