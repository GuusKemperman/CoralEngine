#include "Precomp.h"
#include "Components/EventTestingComponent.h"

#include "Meta/Fwd/MetaPropsFwd.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::EventTestingComponent::OnTick(World& world, entt::entity owner, float)
{
	mLastReceivedWorld = &world;
	mLastReceivedOwner = owner;
	++mNumOfTicks;
	++mTotalNumOfEventsCalled;
}

Engine::MetaType Engine::EventTestingComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<EventTestingComponent>{}, "EventTestingComponent" };
	type.GetProperties().Add(Props::sNoInspectTag);

	BindEvent(type, sTickEvent, &EventTestingComponent::OnTick);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}
