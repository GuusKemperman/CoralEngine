#include "Precomp.h"
#include "Components/UtililtyAi/EnemyAiControllerComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"
#include "World/Registry.h"

Engine::EnemyAiControllerComponent::EnemyAiControllerComponent()
{
}

void Engine::EnemyAiControllerComponent::UpdateState()
{
}

Engine::MetaType Engine::EnemyAiControllerComponent::Reflect()
{
	auto type = MetaType{MetaType::T<EnemyAiControllerComponent>{}, "EnemyAIControllerComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	ReflectComponentType<EnemyAiControllerComponent>(type);
	return type;
}
