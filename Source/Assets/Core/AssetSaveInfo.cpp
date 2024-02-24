#include "Precomp.h"
#include "Assets/Core/AssetSaveInfo.h"

#include "Core/AssetManager.h"
#include "Assets/Core/AssetFileMetaData.h"
#include "Meta/MetaType.h"
#include "Utilities/ClassVersion.h"

Engine::AssetSaveInfo::AssetSaveInfo(const std::string& name, const MetaType& assetClass) :
	mStream(std::ostringstream::binary)
{
	const AssetFileMetaData metaData{  name, assetClass, GetClassVersion(assetClass) };
	metaData.WriteMetaData(mStream);
}

Engine::AssetSaveInfo::AssetSaveInfo(const std::string& name, const MetaType& assetClass,
	const std::filesystem::path& importedFromFile, uint32 importerVersion) :
	mStream(std::ostringstream::binary)
{
	const AssetFileMetaData metaData{ name, assetClass, GetClassVersion(assetClass), AssetFileMetaData::ImporterInfo{ importedFromFile, importerVersion } };
	metaData.WriteMetaData(mStream);
}

Engine::AssetSaveInfo::AssetSaveInfo(AssetSaveInfo&& other) noexcept :
	mStream(std::move(other.mStream))
{
}

Engine::AssetSaveInfo& Engine::AssetSaveInfo::operator=(AssetSaveInfo&& other) noexcept
{
	mStream = std::move(other.mStream);
	return *this;
}

bool Engine::AssetSaveInfo::SaveToFile(const std::filesystem::path& path) const
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

bool Engine::AssetSaveInfo::IsEmpty() const
{
	std::stringstream readStream{ mStream.str(), std::istringstream::binary };
	[[maybe_unused]] const std::optional<AssetFileMetaData> metaData = AssetFileMetaData::ReadMetaData(readStream);
	ASSERT(metaData.has_value());

	// We modify flags we don't care about, not the actual contents of the stream
	return const_cast<std::ostringstream&>(mStream).tellp() - readStream.tellp() == 0;
}

std::string Engine::AssetSaveInfo::ToString() const
{
	return mStream.str();
}
