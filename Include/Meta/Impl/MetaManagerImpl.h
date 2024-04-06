#pragma once
#include "Meta/Fwd/MetaManagerFwd.h"

#include "Meta/MetaType.h"
#include "Meta/MetaReflect.h"
#include "Meta/MetaTypeId.h"
#include "Meta/MetaReflect.h"

template<typename T>
CE::MetaType& CE::MetaManager::GetType()
{
	static_assert(sIsReflectable<T>,
		R"(Type does not have a reflect function, so the type can never be gotten from the MetaManager. 
If it does have a Reflect function, make sure it is included from wherever this error originated.
If you are trying to reflect an std::vector<std::shared_ptr<const Material>>, you need to include Material.h, ReflectVector.h and ReflectSmartPtr.h.)");

	static constexpr TypeId typeId = MakeTypeId<T>();

	if (MetaType* const existingType = TryGetType(typeId);
		existingType != nullptr)
	{
		LIKELY;
		return *existingType;
	}

	MetaType* type{};

	if constexpr (CE::Internal::sHasInternalReflect<T>)
	{
		type = &AddType(ReflectAccess::GetReflectFunc<T>()());
	}
	else
	{
		type = &AddType(Reflector<T>::Reflect());
	}

	ASSERT(type->GetTypeId() == MakeTypeId<T>() && "You have reflected a different type, check what you passed into MetaType constructor");
	return *type;
}