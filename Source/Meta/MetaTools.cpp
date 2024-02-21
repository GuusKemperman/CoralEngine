#include "Precomp.h"
#include "Meta/MetaTools.h"

Engine::MetaAny Engine::MakeRef(MetaAny& anotherAny)
{
	return { anotherAny.GetTypeInfo(), anotherAny.GetData() };
}
