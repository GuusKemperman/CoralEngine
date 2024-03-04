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

void Engine::EventTestingComponent::OnFixedTick(World& world, entt::entity owner)
{
	mLastReceivedWorld = &world;
	mLastReceivedOwner = owner;
	++mNumOfFixedTicks;
	++mTotalNumOfEventsCalled;
}

Engine::MetaType Engine::EventTestingComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<EventTestingComponent>{}, "EventTestingComponent" };
	type.GetProperties().Add(Props::sNoInspectTag);

	type.AddField(&EventTestingComponent::mNumOfTicks, "mNumOfTicks");
	type.AddField(&EventTestingComponent::mNumOfFixedTicks, "mNumOfFixedTicks");
	type.AddField(&EventTestingComponent::mTotalNumOfEventsCalled, "mTotalNumOfEventsCalled");

	BindEvent(type, sTickEvent, &EventTestingComponent::OnTick);
	BindEvent(type, sFixedTickEvent, &EventTestingComponent::OnFixedTick);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}
