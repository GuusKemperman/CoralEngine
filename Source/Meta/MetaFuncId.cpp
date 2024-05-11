#include "Precomp.h"
#include "Meta/MetaFuncId.h"

CE::FuncId CE::MakeFuncId(const TypeTraits returnType, const std::vector<TypeTraits>& parameters)
{
	uint32 typeId = returnType.Hash();

	for (const TypeTraits paramTraits : parameters)
	{
		typeId = Internal::CombineHashes(typeId, paramTraits.Hash());
	}

	return typeId;
}
