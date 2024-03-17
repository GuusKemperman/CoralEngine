#include "Precomp.h"
#include "Components/UtilityAi/EnemyAiControllerComponent.h"

#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::EnemyAiControllerComponent::Reflect()
{
	auto type = MetaType{MetaType::T<EnemyAiControllerComponent>{}, "EnemyAIControllerComponent"};
	MetaProps& props = type.GetProperties();
	props.Add(Props::sIsScriptableTag);
	ReflectComponentType<EnemyAiControllerComponent>(type);
	return type;
}
