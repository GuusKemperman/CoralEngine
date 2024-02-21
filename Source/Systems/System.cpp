#include "Precomp.h"
#include "Systems/System.h"

#include "Meta/MetaType.h"

Engine::MetaType Engine::System::Reflect()
{
    return MetaType{ MetaType::T<System>{}, "System" };
}
