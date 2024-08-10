#include "Precomp.h"
#include "Components/EventTestingComponent.h"

#include "Components/Physics2D/PhysicsBody2DComponent.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"
#include "Meta/Fwd/MetaPropsFwd.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void CE::EmptyEventTestingComponent::OnConstruct(World&, entt::entity)
{
	++sNumOfConstructs;
}

void CE::EmptyEventTestingComponent::OnDestruct(World&, entt::entity)
{
	++sNumOfDestructs;
}

void CE::EmptyEventTestingComponent::OnBeginPlay(World&, entt::entity)
{
	++sNumOfBeginPlays;
}

void CE::EmptyEventTestingComponent::OnTick(World&, entt::entity, float)
{
	++sNumOfTicks;
}

void CE::EmptyEventTestingComponent::OnFixedTick(World&, entt::entity, float)
{
	++sNumOfFixedTicks;
}

void CE::EmptyEventTestingComponent::OnAiTick(World&, entt::entity, float)
{
	++sNumOfAiTicks;
}

float CE::EmptyEventTestingComponent::OnAiEvaluate(const World&, entt::entity)
{
	++sNumOfAiEvaluates;
	return 1000.0f;
}

void CE::EmptyEventTestingComponent::OnCollisionEntry(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++sNumOfCollisionEntry;
}

void CE::EmptyEventTestingComponent::OnCollisionStay(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++sNumOfCollisionStay;
}

void CE::EmptyEventTestingComponent::OnCollisionExit(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++sNumOfCollisionExit;
}

uint32 CE::EmptyEventTestingComponent::GetValue(Name valueName)
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
	case Name::HashString("mNumOfCollisionEntry"): return sNumOfCollisionEntry;
	case Name::HashString("mNumOfCollisionStay"): return sNumOfCollisionStay;
	case Name::HashString("mNumOfCollisionExit"): return sNumOfCollisionExit;
	default: return std::numeric_limits<uint32>::max();
	}
}

void CE::EmptyEventTestingComponent::Reset()
{
	sNumOfTicks = 0;
	sNumOfFixedTicks = 0;
	sNumOfConstructs = 0;
	sNumOfBeginPlays = 0;
	sNumOfDestructs = 0;
	sNumOfAiTicks = 0;
	sNumOfAiEvaluates = 0;
	sNumOfCollisionEntry = 0;
	sNumOfCollisionStay = 0;
	sNumOfCollisionExit = 0;
}

CE::MetaType CE::EmptyEventTestingComponent::Reflect()
{
	auto type = MetaType{MetaType::T<EmptyEventTestingComponent>{}, "EmptyEventTestingComponent"};
	type.GetProperties().Add(Props::sNoInspectTag);

	BindEvent(type, sOnConstruct, &EmptyEventTestingComponent::OnConstruct);
	BindEvent(type, sOnEndPlay, &EmptyEventTestingComponent::OnDestruct);
	BindEvent(type, sOnBeginPlay, &EmptyEventTestingComponent::OnBeginPlay);
	BindEvent(type, sOnTick, &EmptyEventTestingComponent::OnTick);
	BindEvent(type, sOnFixedTick, &EmptyEventTestingComponent::OnFixedTick);
	BindEvent(type, sOnAITick, &EmptyEventTestingComponent::OnAiTick);
	BindEvent(type, sOnAIEvaluate, &EmptyEventTestingComponent::OnAiEvaluate);
	BindEvent(type, sOnCollisionEntry, &EmptyEventTestingComponent::OnCollisionEntry);
	BindEvent(type, sOnCollisionStay, &EmptyEventTestingComponent::OnCollisionStay);
	BindEvent(type, sOnCollisionExit, &EmptyEventTestingComponent::OnCollisionExit);

	ReflectComponentType<EmptyEventTestingComponent>(type);
	return type;
}

void CE::EventTestingComponent::OnConstruct(World&, entt::entity)
{
	++mNumOfConstructs;
}

void CE::EventTestingComponent::OnDestruct(World&, entt::entity)
{
	++mNumOfDestructs;
}

void CE::EventTestingComponent::OnBeginPlay(World&, entt::entity)
{
	++mNumOfBeginPlays;
}

void CE::EventTestingComponent::OnTick(World&, entt::entity, float)
{
	++mNumOfTicks;
}

void CE::EventTestingComponent::OnFixedTick(World&, entt::entity, float)
{
	++mNumOfFixedTicks;
}

void CE::EventTestingComponent::OnAiTick(World&, entt::entity, float)
{
	++mNumOfAiTicks;
}

float CE::EventTestingComponent::OnAiEvaluate(const World&, entt::entity)
{
	++mNumOfAiEvaluates;
	return 0.0f;
}

void CE::EventTestingComponent::OnCollisionEntry(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++mNumOfCollisionEntry;
}

void CE::EventTestingComponent::OnCollisionStay(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++mNumOfCollisionStay;
}

void CE::EventTestingComponent::OnCollisionExit(World&, entt::entity, entt::entity, float, glm::vec2, glm::vec2)
{
	++mNumOfCollisionExit;
}

CE::MetaType CE::EventTestingComponent::Reflect()
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
	type.AddField(&EventTestingComponent::mNumOfCollisionEntry, "mNumOfCollisionEntry");
	type.AddField(&EventTestingComponent::mNumOfCollisionStay, "mNumOfCollisionStay");
	type.AddField(&EventTestingComponent::mNumOfCollisionExit, "mNumOfCollisionExit");

	BindEvent(type, sOnConstruct, &EventTestingComponent::OnConstruct);
	BindEvent(type, sOnBeginPlay, &EventTestingComponent::OnBeginPlay);
	BindEvent(type, sOnTick, &EventTestingComponent::OnTick);
	BindEvent(type, sOnFixedTick, &EventTestingComponent::OnFixedTick);
	BindEvent(type, sOnEndPlay, &EventTestingComponent::OnDestruct);
	BindEvent(type, sOnAITick, &EventTestingComponent::OnAiTick);
	BindEvent(type, sOnAIEvaluate, &EventTestingComponent::OnAiEvaluate);
	BindEvent(type, sOnCollisionEntry, &EventTestingComponent::OnCollisionEntry);
	BindEvent(type, sOnCollisionStay, &EventTestingComponent::OnCollisionStay);
	BindEvent(type, sOnCollisionExit, &EventTestingComponent::OnCollisionExit);

	ReflectComponentType<EventTestingComponent>(type);
	return type;
}
