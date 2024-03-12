#include "Precomp.h"
#include "Components/UtililtyAi/BaseIdleState.h"

#include "Meta/MetaType.h"

Engine::MetaType Engine::BaseIdleState::Reflect()
{
	auto type = MetaType{MetaType::T<BaseIdleState>{}, "BaseIdleState"};
	//type.GetProperties().Add(Props::sNoInspectTag);

	/*BindEvent(type, sAITickEvent, &EmptyEventTestingComponent::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &EmptyEventTestingComponent::OnAiEvaluate);*/

	//ReflectComponentType<EventTestingComponent>(type);
	return type;
}
