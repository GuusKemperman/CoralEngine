#include "Precomp.h"
#include "BasicDataTypes/Colors/LinearColor.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"
#include "Utilities/Reflect/ReflectFieldType.h"

Engine::MetaType Engine::LinearColor::Reflect()
{
    MetaType type = MetaType{ MetaType::T<LinearColor>{}, "LinearColor" };
    type.GetProperties().Add(Props::sIsScriptableTag).Add(Props::sIsScriptOwnableTag);
    ReflectFieldType<LinearColor>(type);
    return type;
}
