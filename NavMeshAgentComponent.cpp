#include "Precomp.h"
#include "NavMeshAgentComponent.h"

Engine::NavMeshAgentComponent::NavMeshAgentComponent(const float walkingSpeed) : Speed(walkingSpeed)
{
}

float Engine::NavMeshAgentComponent::GetSpeed() const
{
	return Speed;
}
