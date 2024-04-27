#pragma once
#ifdef EDITOR

#include "Assets/Core/AssetHandle.h"

namespace CE
{
	class Texture;
	class Script;

	AssetHandle<Texture> GetThumbNail(WeakAssetHandle<> forAsset);

	namespace Internal
	{
		AssetHandle<Texture> GetDefaultThumbnail();

		static constexpr std::string_view sGetThumbnailFuncName = "GetThumbNail";
	}
}

template<typename T>
CE::AssetHandle<CE::Texture> GetThumbNailImpl([[maybe_unused]] const CE::WeakAssetHandle<T>& forAsset)
{
	return CE::Internal::GetDefaultThumbnail();
}

template<>
CE::AssetHandle<CE::Texture> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Texture>& forAsset);

template<>
CE::AssetHandle<CE::Texture> GetThumbNailImpl(const CE::WeakAssetHandle<CE::Script>& forAsset);

#endif // EDITOR