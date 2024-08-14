#include "Precomp.h"
#include "Assets/Core/AssetSaveInfo.h"

#include "Core/AssetManager.h"
#include "Assets/Core/AssetMetaData.h"
#include "Utilities/ClassVersion.h"

CE::AssetSaveInfo::AssetSaveInfo(const std::string& name, const MetaType& assetClass,
	const std::optional<AssetMetaData::ImporterInfo>& importerInfo) :
	mStream(std::ostringstream::binary),
	mMetaData(name, assetClass, GetClassVersion(assetClass), importerInfo)
{
	mMetaData.WriteMetaData(mStream);
}

bool CE::AssetSaveInfo::SaveToFile(const std::filesystem::path& path) const
{
	if (path.extension() != AssetManager::sAssetExtension)
	{
		LOG(LogAssets, Message, "Failed to save asset: {} did not have extension {}", path.string(), AssetManager::sAssetExtension);
		return false;
	}

	std::error_code err{};
	create_directories(path.parent_path(), err);

	if (err)
	{
		LOG(LogAssets, Message, "Failed to save asset: failed to create directory {} - {}", path.parent_path().string(), err.message());
		return false;
	}

	std::ofstream fstream{ path, std::ofstream::binary };

	if (!fstream.is_open())
	{
		LOG(LogAssets, Message, "Failed to save asset: {} could not be written to", path.string());
		return false;
	}

	fstream << mStream.str();

	return true;
}

bool CE::AssetSaveInfo::IsEmpty() const
{
	std::stringstream readStream{ mStream.str(), std::istringstream::binary };
	[[maybe_unused]] const std::optional<AssetMetaData> metaData = AssetMetaData::ReadMetaData(readStream);
	ASSERT(metaData.has_value());

	// We modify flags we don't care about, not the actual contents of the stream
	return const_cast<std::ostringstream&>(mStream).tellp() - readStream.tellp() == 0;
}

std::string CE::AssetSaveInfo::ToString() const
{
	return mStream.str();
}
