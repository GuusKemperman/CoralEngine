#include "Precomp.h"
#include "Components/EventTestingComponent.h"

#include "Meta/Fwd/MetaPropsFwd.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::EmptyEventTestingComponent::OnConstruct(World&, entt::entity)
{
	++sNumOfConstructs;
}

void Engine::EmptyEventTestingComponent::OnBeginPlay(World&, entt::entity)
{
	++sNumOfBeginPlays;
}

void Engine::EmptyEventTestingComponent::OnTick(World&, entt::entity, float)
{
	++sNumOfTicks;
}

void Engine::EmptyEventTestingComponent::OnFixedTick(World&, entt::entity)
{
	++sNumOfFixedTicks;
}

uint32 Engine::EmptyEventTestingComponent::GetValue(Name valueName)
{
	switch(valueName.GetHash())
	{
	case Name::HashString("mNumOfTicks"): return sNumOfTicks;
	case Name::HashString("mNumOfFixedTicks"): return sNumOfFixedTicks;
	case Name::HashString("mNumOfConstructs"): return sNumOfConstructs;
	case Name::HashString("mNumOfBeginPlays"): return sNumOfBeginPlays;
	default: return std::numeric_limits<uint32>::max();
	}
}

void Engine::EmptyEventTestingComponent::Reset()
{
	sNumOfTicks = 0;
	sNumOfFixedTicks = 0;
	sNumOfConstructs = 0;
	sNumOfBeginPlays = 0;
}

Engine::MetaType Engine::EmptyEventTestingComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<EmptyEventTestingComponent>{}, "EmptyEventTestingComponent" };
	type.GetProperties().Add(Props::sNoInspectTag);

	BindEvent(type, sConstructEvent, &EmptyEventTestingComponent::OnConstruct);
	BindEvent(type, sBeginPlayEvent, &EmptyEventTestingComponent::OnBeginPlay);
	BindEvent(type, sTickEvent, &EmptyEventTestingComponent::OnTick);
	BindEvent(type, sFixedTickEvent, &EmptyEventTestingComponent::OnFixedTick);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}

void Engine::EventTestingComponent::OnConstruct(World&, entt::entity)
{
	++mNumOfConstructs;
}

void Engine::EventTestingComponent::OnBeginPlay(World&, entt::entity)
{
	++mNumOfBeginPlays;
}

void Engine::EventTestingComponent::OnTick(World&, entt::entity, float)
{
	++mNumOfTicks;
}

void Engine::EventTestingComponent::OnFixedTick(World&, entt::entity)
{
	++mNumOfFixedTicks;
}

Engine::MetaType Engine::EventTestingComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<EventTestingComponent>{}, "EventTestingComponent" };
	type.GetProperties().Add(Props::sNoInspectTag);

	type.AddField(&EventTestingComponent::mNumOfTicks, "mNumOfTicks");
	type.AddField(&EventTestingComponent::mNumOfFixedTicks, "mNumOfFixedTicks");

	BindEvent(type, sConstructEvent, &EventTestingComponent::OnConstruct);
	BindEvent(type, sBeginPlayEvent, &EventTestingComponent::OnBeginPlay);
	BindEvent(type, sTickEvent, &EventTestingComponent::OnTick);
	BindEvent(type, sFixedTickEvent, &EventTestingComponent::OnFixedTick);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}
