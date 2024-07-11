#include "Precomp.h"
#include "Components/XPOrbComponent.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectComponentType.h"

CE::MetaType Game::XPOrbComponent::Reflect()
{
	CE::MetaType type{ CE::MetaType::T<XPOrbComponent>{}, "XPOrbComponent" };
	CE::MetaProps& props = type.GetProperties();
	props.Add(CE::Props::sIsScriptableTag);
	props.Set(CE::Props::sOldNames, "XPOrbScript");
	CE::ReflectComponentType<XPOrbComponent>(type);
	return type;
}
