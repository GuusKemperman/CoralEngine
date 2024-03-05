#include "Precomp.h"
#include "Components/UtililtyAi/BaseIdleState.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::BaseIdleState::Reflect()
{
	auto metaType = MetaType{MetaType::T<BaseIdleState>{}, "BaseIdleState"};
	metaType.GetProperties().Add("AiState");

	ReflectComponentType<BaseIdleState>(metaType);

	return metaType;
}
