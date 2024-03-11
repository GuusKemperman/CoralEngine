#include "Precomp.h"
#include "Components/UtililtyAi/States/AttackingState.h"

#include "Meta/MetaType.h"
#include "Utilities/Events.h"
#include "Utilities/Reflect/ReflectComponentType.h"

void Engine::AttackingState::OnAiTick(World&, entt::entity, float)
{
}

float Engine::AttackingState::OnAiEvaluate(const World&, entt::entity)
{
	return 0.0f;
}

Engine::MetaType Engine::AttackingState::Reflect()
{
	auto type = MetaType{MetaType::T<AttackingState>{}, "AttackingState"};
	type.GetProperties().Add(Props::sIsScriptableTag);

	BindEvent(type, sAITickEvent, &AttackingState::OnAiTick);
	BindEvent(type, sAIEvaluateEvent, &AttackingState::OnAiEvaluate);

	ReflectComponentType<AttackingState>(type);
	return type;
}
