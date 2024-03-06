#include "Precomp.h"
#include "Components/UtililtyAi/BaseIdleState.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectComponentType.h"

Engine::MetaType Engine::BaseIdleState::Reflect()
{
	return MetaType{MetaType::T<BaseIdleState>{}, "BaseIdleState"};
}
