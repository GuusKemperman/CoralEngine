#include "Precomp.h"
#include "Utilities/ClassVersion.h"

#include "Meta/MetaType.h"
#include "Meta/MetaProps.h"

uint32 CE::GetClassVersion(const MetaType& type)
{
    return type.GetProperties().TryGetValue<uint32>(Name{ Props::sVersion }).value_or(0);
}

void CE::SetClassVersion(MetaType& type, uint32 version)
{
	type.GetProperties().Set(Name{ Props::sVersion }, version);
}

