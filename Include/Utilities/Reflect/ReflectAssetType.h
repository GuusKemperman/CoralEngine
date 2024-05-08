#pragma once
#include "Meta/MetaType.h"
#include "Assets/Asset.h"
#include "Assets/Core/AssetHandle.h"
#include "Meta/ReflectedTypes/STD/ReflectVector.h"

#ifdef EDITOR
#include "EditorSystems/ThumbnailEditorSystem.h"
#endif // EDITOR

namespace CE
{
	namespace Props
	{
		/*
		Use on:
			Assets

		Description:
			Can be used to indicate that assets of this type will never have dependencies
			on other assets. This means they do not need to be unloaded when the editor
			refreshes, and can lead to performance benefits.

		Example:
			type.GetProperties().Add(Props::sCannotReferenceOtherAssetsTag)
		*/
		static constexpr std::string_view sCannotReferenceOtherAssetsTag = "sCannotReferenceOtherAssetsTag";
	}

	// Makes sure the type receives all the functionality that an Asset requires.
	template<typename T>
	void ReflectAssetType([[maybe_unused]] MetaType& type)
	{
#ifdef EDITOR
		type.AddFunc(
			[](GetThumbnailRet& ret, const WeakAssetHandle<>& asset)
			{
				ret = GetThumbNailImpl<T>(StaticAssetHandleCast<T>(asset));
			},
			Internal::sGetThumbnailFuncName,
			MetaFunc::ExplicitParams<GetThumbnailRet&, const WeakAssetHandle<>&>{});
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
