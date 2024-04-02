#include "Precomp.h"
#include "Meta/MetaTools.h"

CE::MetaAny CE::MakeRef(MetaAny& anotherAny)
{
	return { anotherAny.GetTypeInfo(), anotherAny.GetData() };
}
