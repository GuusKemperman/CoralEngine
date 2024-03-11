#include "Precomp.h"
#include "Components/UtililtyAi/States/IdleState.h"

#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::IdleState::OnAiTick(World&, entt::entity, float)
{
}

float Engine::IdleState::OnAiEvaluate(const World&, entt::entity)
{
	return 0.0f;
}

Engine::MetaType Engine::IdleState::Reflect()
{
	auto type = MetaType{MetaType::T<IdleState>{}, "IdleState"};
	type.GetProperties().Add(Props::sIsScriptableTag);

	BindEvent(type, sAITickEvent, &IdleState::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &IdleState::OnAiEvaluate);

	ReflectComponentType<IdleState>(type);
	return type;
}
