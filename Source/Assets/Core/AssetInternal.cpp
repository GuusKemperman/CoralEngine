#include "Precomp.h"
#include "Assets/Core/AssetInternal.h"

CE::Internal::AssetInternal::AssetInternal(AssetFileMetaData&& metaData, const std::optional<std::filesystem::path>& path) :
	mMetaData(std::move(metaData)),
	mFileOfOrigin(path)
{
}