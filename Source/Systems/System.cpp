#include "Precomp.h"
#include "Systems/System.h"

#include "Meta/MetaType.h"

CE::MetaType CE::System::Reflect()
{
    return MetaType{ MetaType::T<System>{}, "System" };
}
