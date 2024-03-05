#include "Precomp.h"
#include "Components/EventTestingComponent.h"

#include "Meta/Fwd/MetaPropsFwd.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::EmptyEventTestingComponent::OnTick(World&, entt::entity, float)
{
	++sNumOfTicks;
	++sTotalNumOfEventsCalled;
}

void Engine::EmptyEventTestingComponent::OnFixedTick(World&, entt::entity)
{
	++sNumOfFixedTicks;
	++sTotalNumOfEventsCalled;
}

uint32 Engine::EmptyEventTestingComponent::GetValue(Name valueName)
{
	switch(valueName.GetHash())
	{
	case Name::HashString("mNumOfTicks"): return sNumOfTicks;
	case Name::HashString("mNumOfFixedTicks"): return sNumOfFixedTicks;
	case Name::HashString("mTotalNumOfEventsCalled"): return sTotalNumOfEventsCalled;
	default: return std::numeric_limits<uint32>::max();
	}
}

void Engine::EmptyEventTestingComponent::Reset()
{
	sNumOfTicks = 0;
	sNumOfFixedTicks = 0;
	sTotalNumOfEventsCalled = 0;
}

Engine::MetaType Engine::EmptyEventTestingComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<EmptyEventTestingComponent>{}, "EmptyEventTestingComponent" };
	type.GetProperties().Add(Props::sNoInspectTag);

	BindEvent(type, sTickEvent, &EmptyEventTestingComponent::OnTick);
	BindEvent(type, sFixedTickEvent, &EmptyEventTestingComponent::OnFixedTick);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}

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
