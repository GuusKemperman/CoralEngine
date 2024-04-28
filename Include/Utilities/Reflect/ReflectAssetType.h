#pragma once
#include "Meta/MetaType.h"
#include "Assets/Asset.h"
#include "Assets/Core/AssetHandle.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

#ifdef EDITOR
#include "Assets/Core/AssetThumbnails.h"
#endif // EDITOR

namespace CE
{
	// Makes sure the type receives all the functionality that an Asset requires.
	template<typename T>
	void ReflectAssetType([[maybe_unused]] MetaType& type)
	{
#ifdef EDITOR
		type.AddFunc(
			[](std::unique_ptr<ThumbnailFactory>& factory, const WeakAssetHandle<>& asset)
			{
				factory = GetThumbNailImpl<T>(StaticAssetHandleCast<T>(asset));
			},
			Internal::sGetThumbnailFuncName,
			MetaFunc::ExplicitParams<std::unique_ptr<ThumbnailFactory>&, const WeakAssetHandle<>&>{});
#endif // EDITOR

		// Will ensure the array and ptr types are reflected at startup
		// as well, as they are exposed to scripting.
		if (!Internal::ReflectAtStartup<std::vector<AssetHandle<T>>>::sDummy
			&& !Internal::ReflectAtStartup<AssetHandle<T>>::sDummy
			&& !Internal::ReflectAtStartup<WeakAssetHandle<T>>::sDummy)
		{
			ASSERT(false);
			// Just to prevent the compiler from optimising it out
			putchar(0);
		}
	}
}
