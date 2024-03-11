#include "Precomp.h"
#include "Components/UtililtyAi/States/ChasingState.h"

#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::ChasingState::OnAiTick(World&, entt::entity, float)
{
}

float Engine::ChasingState::OnAiEvaluate(const World&, entt::entity)
{
	return 0.0f;
}

Engine::MetaType Engine::ChasingState::Reflect()
{
	auto type = MetaType{MetaType::T<ChasingState>{}, "ChasingState"};
	type.GetProperties().Add(Props::sIsScriptableTag);

	BindEvent(type, sAITickEvent, &ChasingState::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &ChasingState::OnAiEvaluate);

	ReflectComponentType<ChasingState>(type);
	return type;
}
