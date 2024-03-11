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

void Engine::EmptyEventTestingComponent::OnAiTick(World&, entt::entity, float)
{
	++sNumOfAiTicks;
}

float Engine::EmptyEventTestingComponent::OnAiEvaluate(const World&, entt::entity)
{
	++sNumOfAiEvaluates;
	return 1000.0f;
}

uint32 Engine::EmptyEventTestingComponent::GetValue(Name valueName)
{
	switch (valueName.GetHash())
	{
	case Name::HashString("mNumOfTicks"): return sNumOfTicks;
	case Name::HashString("mNumOfFixedTicks"): return sNumOfFixedTicks;
	case Name::HashString("mNumOfConstructs"): return sNumOfConstructs;
	case Name::HashString("mNumOfBeginPlays"): return sNumOfBeginPlays;
	case Name::HashString("mNumOfDestructs"): return sNumOfDestructs;
	case Name::HashString("mNumOfAiTicks"): return sNumOfAiTicks;
	case Name::HashString("mNumOfAiEvaluates"): return sNumOfAiEvaluates;
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
	sNumOfAiTicks = 0;
	sNumOfAiEvaluates = 0;
}

Engine::MetaType Engine::EmptyEventTestingComponent::Reflect()
{
	auto type = MetaType{MetaType::T<EmptyEventTestingComponent>{}, "EmptyEventTestingComponent"};
	type.GetProperties().Add(Props::sNoInspectTag);

	BindEvent(type, sConstructEvent, &EmptyEventTestingComponent::OnConstruct);
	BindEvent(type, sDestructEvent, &EmptyEventTestingComponent::OnDestruct);
	BindEvent(type, sBeginPlayEvent, &EmptyEventTestingComponent::OnBeginPlay);
	BindEvent(type, sTickEvent, &EmptyEventTestingComponent::OnTick);
	BindEvent(type, sFixedTickEvent, &EmptyEventTestingComponent::OnFixedTick);
	BindEvent(type, sAITickEvent, &EmptyEventTestingComponent::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &EmptyEventTestingComponent::OnAiEvaluate);

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

void Engine::EventTestingComponent::OnAiTick(World&, entt::entity, float)
{
	++mNumOfAiTicks;
}

float Engine::EventTestingComponent::OnAiEvaluate(const World&, entt::entity) const
{
	++mNumOfAiEvaluates;
	return 0.0f;
}

Engine::MetaType Engine::EventTestingComponent::Reflect()
{
	auto type = MetaType{MetaType::T<EventTestingComponent>{}, "EventTestingComponent"};
	type.GetProperties().Add(Props::sNoInspectTag);

	type.AddField(&EventTestingComponent::mNumOfTicks, "mNumOfTicks");
	type.AddField(&EventTestingComponent::mNumOfFixedTicks, "mNumOfFixedTicks");
	type.AddField(&EventTestingComponent::mNumOfConstructs, "mNumOfConstructs");
	type.AddField(&EventTestingComponent::mNumOfBeginPlays, "mNumOfBeginPlays");
	type.AddField(&EventTestingComponent::mNumOfDestructs, "mNumOfDestructs");
	type.AddField(&EventTestingComponent::mNumOfAiTicks, "mNumOfAiTicks");
	type.AddField(&EventTestingComponent::mNumOfAiEvaluates, "mNumOfAiEvaluates");

	BindEvent(type, sConstructEvent, &EventTestingComponent::OnConstruct);
	BindEvent(type, sBeginPlayEvent, &EventTestingComponent::OnBeginPlay);
	BindEvent(type, sTickEvent, &EventTestingComponent::OnTick);
	BindEvent(type, sFixedTickEvent, &EventTestingComponent::OnFixedTick);
	BindEvent(type, sDestructEvent, &EventTestingComponent::OnDestruct);
	BindEvent(type, sAITickEvent, &EventTestingComponent::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &EventTestingComponent::OnAiEvaluate);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}
