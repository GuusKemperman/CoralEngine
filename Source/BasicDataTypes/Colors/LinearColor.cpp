#include "Precomp.h"
#include "BasicDataTypes/Colors/LinearColor.h"

#include "Meta/MetaType.h"
#include "Utilities/Reflect/ReflectFieldType.h"

Engine::MetaType Engine::LinearColor::Reflect()
{
    MetaType type = MetaType{ MetaType::T<LinearColor>{}, "LinearColor" };
    ReflectFieldType<LinearColor>(type);
    return type;
}
