#include "Precomp.h"
#include "Components/PlayerComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

CE::MetaType CE::PlayerComponent::Reflect()
{
	auto metaType = CE::MetaType{CE::MetaType::T<PlayerComponent>{}, "PlayerComponent"};
	metaType.GetProperties().Add(CE::Props::sIsScriptableTag);

	metaType.AddField(&PlayerComponent::mID, "mID").GetProperties().Add(CE::Props::sIsScriptableTag);

	CE::ReflectComponentType<PlayerComponent>(metaType);

	return metaType;
}
