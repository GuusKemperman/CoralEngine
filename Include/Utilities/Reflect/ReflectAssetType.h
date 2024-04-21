#pragma once
#include "Meta/MetaType.h"
#include "Assets/Asset.h"
#include "Assets/Core/AssetHandle.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

namespace CE
{
	// Makes sure the type receives all the functionality that an Asset requires.
	template<typename AssetT>
	void ReflectAssetType(MetaType&)
	{
		using RefType = AssetHandle<AssetT>;
		using ArrType = std::vector<RefType>;

		// Will ensure the array and ptr types are reflected at startup
		// as well, as they are exposed to scripting.
		if (!Internal::ReflectAtStartup<ArrType>::sDummy
			&& !Internal::ReflectAtStartup<RefType>::sDummy)
		{
			ASSERT(false);
			// Just to prevent the compiler from optimising it out
			putchar(0);
		}
	}
}
