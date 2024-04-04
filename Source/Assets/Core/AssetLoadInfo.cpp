#include "Precomp.h"
#include "Assets/Core/AssetLoadInfo.h"

#include "Assets/Core/AssetFileMetaData.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Meta/MetaType.h"
#include "Assets/Asset.h"
#include "Utilities/ClassVersion.h"

bool CE::AssetLoadInfo::ConstructFromCurrentStream()
{
	const std::optional<AssetFileMetaData> metaData = AssetFileMetaData::ReadMetaData(*mStream);

	if (!metaData.has_value())
	{
		return false;
	}

	mMetaData = std::make_unique<AssetFileMetaData>(*metaData);
	return true;
}

CE::AssetLoadInfo::AssetLoadInfo(const std::filesystem::path& fromFile) :
	mStream(std::make_unique<std::ifstream>(fromFile, std::ifstream::binary))
{
	if (!static_cast<const std::ifstream*>(mStream.get())->is_open())
	{
		LOG(LogAssets, Message, "Failed to produce valid AssetLoadInfo: the file {} could not be opened", fromFile.string());
		return;
	}

	if (!ConstructFromCurrentStream())
	{
		LOG(LogAssets, Message, "Failed to produce valid AssetLoadInfo: the file {} did not contain valid metadata", fromFile.string());
	}
}

CE::AssetLoadInfo::AssetLoadInfo(std::unique_ptr<std::istream> stream) :
	mStream(std::move(stream))
{
	if (!ConstructFromCurrentStream())
	{
		LOG(LogAssets, Message, "Failed to produce valid AssetLoadInfo: the stream did not contain valid metadata");
	}
}

CE::AssetLoadInfo::AssetLoadInfo(const AssetSaveInfo& saveInfo)
{
	mStream = std::make_unique<std::istringstream>(saveInfo.ToString());

	[[maybe_unused]] const bool success = ConstructFromCurrentStream();
	ASSERT(success);
}

CE::AssetLoadInfo::AssetLoadInfo(const MetaType& assetClass, const std::string_view name) :
	mStream(std::make_unique<std::istringstream>()),
	mMetaData(std::make_unique<AssetFileMetaData>(AssetFileMetaData{ name, assetClass, GetClassVersion(assetClass) }))
{	
	if (!assetClass.IsDerivedFrom<Asset>())
	{
		LOG(LogAssets, Error, "AssetLoadInfo was constructed with object class {}, but this class does not derive from Asset",
			assetClass.GetName());
	}
}

std::optional<CE::AssetLoadInfo> CE::AssetLoadInfo::LoadFromFile(const std::filesystem::path& fromFile)
{
	std::optional<AssetLoadInfo> loadInfo{ AssetLoadInfo{fromFile} };

	if (loadInfo->mMetaData == nullptr)
	{
		return std::nullopt;
	}
	return loadInfo;
}

std::optional<CE::AssetLoadInfo> CE::AssetLoadInfo::LoadFromStream(std::unique_ptr<std::istream> stream)
{
	std::optional<AssetLoadInfo> loadInfo{ AssetLoadInfo{std::move(stream)}};

	if (loadInfo->mMetaData == nullptr)
	{
		return std::nullopt;
	}
	return loadInfo;
}

