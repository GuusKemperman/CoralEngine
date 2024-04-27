#include "Precomp.h"
#include "Assets/Core/AssetThumbnails.h"

#include "Core/AssetManager.h"

CE::AssetHandle<CE::Texture> CE::GetThumbNail(WeakAssetHandle<> forAsset)
{
	if (forAsset == nullptr)
	{
		return nullptr;
	}

	const MetaType& assetType = forAsset.GetMetaData().GetClass();
	const MetaFunc* const func = assetType.TryGetFunc(Internal::sGetThumbnailFuncName);

	if (func == nullptr
		|| func->GetParameters().size() != 1)
	{
		LOG(LogEditor, Error, "Could not get thumbnail for asset {} of type {}, function did not exist or had unexpected parameters",
			forAsset.GetMetaData().GetName(),
			assetType.GetName());
		return nullptr;
	}

	const MetaType* handleType = MetaManager::Get().TryGetType(func->GetParameters()[0].mTypeTraits.mStrippedTypeId);

	if (handleType == nullptr)
	{
		LOG(LogEditor, Error, "Could not get thumbnail for asset {} of type {}, could not retrieve handle type",
			forAsset.GetMetaData().GetName(),
			assetType.GetName());
		return nullptr;
	}

	AssetHandle<Texture> returnValue{};
	const FuncResult result = func->InvokeUncheckedUnpackedWithRVO(&returnValue, MetaAny{ *handleType, &forAsset, false });

	if (result.HasError())
	{
		LOG(LogEditor, Error, "Could not get thumbnail for asset {} of type {}, function returned error {}",
			forAsset.GetMetaData().GetName(),
			assetType.GetName(),
			result.Error());
	}
	return returnValue;
}

CE::AssetHandle<CE::Texture> CE::Internal::GetDefaultThumbnail()
{
	return AssetManager::Get().TryGetAsset<Texture>("T_DefaultIcon");
}

template <>
CE::AssetHandle<CE::Texture> GetThumbNailImpl<CE::Texture>(const CE::WeakAssetHandle<CE::Texture>& forAsset)
{
	return CE::AssetHandle<CE::Texture>{ forAsset };
}

template <>
CE::AssetHandle<CE::Texture> GetThumbNailImpl<CE::Script>(const CE::WeakAssetHandle<CE::Script>&)
{
	return CE::AssetManager::Get().TryGetAsset<CE::Texture>("T_ScriptIcon");
}
