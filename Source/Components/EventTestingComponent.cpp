#include "Precomp.h"
#include "Components/EventTestingComponent.h"

#include "Meta/Fwd/MetaPropsFwd.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::EmptyEventTestingComponent::OnConstruct(World&, entt::entity)
{
	++sNumOfConstructs;
}

void Engine::EmptyEventTestingComponent::OnDestruct(World&, entt::entity)
{
	++sNumOfDestructs;
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

void Engine::EmptyEventTestingComponent::OnCollisionEntry(World&, entt::entity, entt::entity, float, glm::vec2,
	glm::vec2)
{
	++sNumOfCollisionEntry;
}

void Engine::EmptyEventTestingComponent::OnCollisionStay(World&, entt::entity, entt::entity, float, glm::vec2,
	glm::vec2)
{
	++sNumOfCollisionStay;
}

void Engine::EmptyEventTestingComponent::OnCollisionExit(World&, entt::entity, entt::entity, float, glm::vec2,
	glm::vec2)
{
	++sNumOfCollisionExit;
}

uint32 Engine::EmptyEventTestingComponent::GetValue(Name valueName)
{
	switch(valueName.GetHash())
	{
	case Name::HashString("mNumOfTicks"): return sNumOfTicks;
	case Name::HashString("mNumOfFixedTicks"): return sNumOfFixedTicks;
	case Name::HashString("mNumOfConstructs"): return sNumOfConstructs;
	case Name::HashString("mNumOfBeginPlays"): return sNumOfBeginPlays;
	case Name::HashString("mNumOfDestructs"): return sNumOfDestructs;
	case Name::HashString("mNumOfCollisionEntry"): return sNumOfCollisionEntry;
	case Name::HashString("mNumOfCollisionStay"): return sNumOfCollisionStay;
	case Name::HashString("mNumOfCollisionExit"): return sNumOfCollisionExit;
	default: return std::numeric_limits<uint32>::max();
	}
}

void Engine::EmptyEventTestingComponent::Reset()
{
	sNumOfTicks = 0;
	sNumOfFixedTicks = 0;
	sNumOfConstructs = 0;
	sNumOfBeginPlays = 0;
	sNumOfDestructs = 0;
	sNumOfCollisionEntry = 0;
	sNumOfCollisionStay = 0;
	sNumOfCollisionExit = 0;
}

Engine::MetaType Engine::EmptyEventTestingComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<EmptyEventTestingComponent>{}, "EmptyEventTestingComponent" };
	type.GetProperties().Add(Props::sNoInspectTag);

	BindEvent(type, sConstructEvent, &EmptyEventTestingComponent::OnConstruct);
	BindEvent(type, sDestructEvent, &EmptyEventTestingComponent::OnDestruct);
	BindEvent(type, sBeginPlayEvent, &EmptyEventTestingComponent::OnBeginPlay);
	BindEvent(type, sTickEvent, &EmptyEventTestingComponent::OnTick);
	BindEvent(type, sFixedTickEvent, &EmptyEventTestingComponent::OnFixedTick);
	BindEvent(type, sCollisionEntryEvent, &EmptyEventTestingComponent::OnCollisionEntry);
	BindEvent(type, sCollisionStayEvent, &EmptyEventTestingComponent::OnCollisionStay);
	BindEvent(type, sCollisionExitEvent, &EmptyEventTestingComponent::OnCollisionExit);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}

void Engine::EventTestingComponent::OnConstruct(World&, entt::entity)
{
	++mNumOfConstructs;
}

void Engine::EventTestingComponent::OnDestruct(World&, entt::entity)
{
	++mNumOfDestructs;
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

void Engine::EventTestingComponent::OnCollisionEntry(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++mNumOfCollisionEntry;
}

void Engine::EventTestingComponent::OnCollisionStay(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++mNumOfCollisionStay;
}

void Engine::EventTestingComponent::OnCollisionExit(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++mNumOfCollisionExit;
}

Engine::MetaType Engine::EventTestingComponent::Reflect()
{
	MetaType type = MetaType{ MetaType::T<EventTestingComponent>{}, "EventTestingComponent" };
	type.GetProperties().Add(Props::sNoInspectTag);

	type.AddField(&EventTestingComponent::mNumOfTicks, "mNumOfTicks");
	type.AddField(&EventTestingComponent::mNumOfFixedTicks, "mNumOfFixedTicks");
	type.AddField(&EventTestingComponent::mNumOfConstructs, "mNumOfConstructs");
	type.AddField(&EventTestingComponent::mNumOfBeginPlays, "mNumOfBeginPlays");
	type.AddField(&EventTestingComponent::mNumOfDestructs, "mNumOfDestructs");
	type.AddField(&EventTestingComponent::mNumOfCollisionEntry, "mNumOfCollisionEntry");
	type.AddField(&EventTestingComponent::mNumOfCollisionStay, "mNumOfCollisionStay");
	type.AddField(&EventTestingComponent::mNumOfCollisionExit, "mNumOfCollisionExit");

	BindEvent(type, sConstructEvent, &EventTestingComponent::OnConstruct);
	BindEvent(type, sBeginPlayEvent, &EventTestingComponent::OnBeginPlay);
	BindEvent(type, sTickEvent, &EventTestingComponent::OnTick);
	BindEvent(type, sFixedTickEvent, &EventTestingComponent::OnFixedTick);
	BindEvent(type, sDestructEvent, &EventTestingComponent::OnDestruct);
	BindEvent(type, sCollisionEntryEvent, &EventTestingComponent::OnCollisionEntry);
	BindEvent(type, sCollisionStayEvent, &EventTestingComponent::OnCollisionStay);
	BindEvent(type, sCollisionExitEvent, &EventTestingComponent::OnCollisionExit);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}
