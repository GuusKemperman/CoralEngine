#pragma once
#include "Meta/Fwd/MetaReflectFwd.h"

#include "Meta/MetaTypeId.h"

#include "Meta/ReflectedTypes/ReflectPrimitiveTypes.h"
#include "Meta/ReflectedTypes/ReflectGLM.h"
#include "Meta/ReflectedTypes/STD/ReflectString.h"
#include "Meta/ReflectedTypes/ReflectENTT.h"
#include "Meta/ReflectedTypes/ReflectNull.h"

template<typename T>
bool CE::Internal::InitStaticDummy()
{
	static_assert(!sHasExternalReflect<T> || !sHasInternalReflect<T>, "Both an internal and external reflect function.");
	static_assert(sHasExternalReflect<T> || sHasInternalReflect<T>,
		R"(No external or internal reflect function. You need to make sure the Reflect function is included from wherever you are trying to reflect it.
If you are trying to reflect an std::vector<AssetHandle<Material>>, you need to include AssetHandle.h, Material.h and ReflectVector.h.)");

	if constexpr (sHasInternalReflect<T>)
	{
		Internal::RegisterReflectFunc(MakeTypeId<T>(), ReflectAccess::GetReflectFunc<T>());
	}
	else if constexpr (sHasExternalReflect<T>)
	{
		Internal::RegisterReflectFunc(MakeTypeId<T>(), &Reflector<T>::Reflect);
	}
	return true;
}