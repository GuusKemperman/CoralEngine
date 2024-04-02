#include "Precomp.h"
#include "Assets/Core/AssetFileMetaData.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Assets/Asset.h"
#include "GSON/GSONBinary.h"
#include "Utilities/ClassVersion.h"

CE::AssetFileMetaData::AssetFileMetaData(std::string_view name, const MetaType& assetClass, uint32 assetVersion,
	const std::optional<ImporterInfo>& importerInfo) :
	mAssetVersion(assetVersion == std::numeric_limits<uint32>::max() ? GetClassVersion(assetClass) : assetVersion),
	mAssetName(name),
	mClass(assetClass),
	mImporterInfo(importerInfo)
{
	ASSERT(mClass.get().IsDerivedFrom<Asset>());
}

template<typename T>
static std::optional<T> TryRead(std::istream& fromStream)
{
	T tmp{};
	fromStream.read(reinterpret_cast<char*>(&tmp), sizeof(T));
	const std::streamsize amountRead = fromStream.gcount();

	if (amountRead != sizeof(T))
	{
		return std::optional<T>{};
	}
	return tmp;
}

std::optional<CE::AssetFileMetaData> CE::AssetFileMetaData::ReadMetaData(std::istream& fromStream)
{
	std::optional<uint32> version = TryRead<uint32>(fromStream);

	if (!version.has_value())
	{
		LOG(LogAssets, Message, "Asset metadata was empty");
		return std::nullopt;
	}

	switch(*version)
	{
	case 0: 
		return ReadMetaDataV0(fromStream, *version);
	case 1:
	case 2:
		return ReadMetaDataV1V2(fromStream, *version);
	default:
		LOG(LogAssets, Message, "Asset metadata version {} is not recognised and not supported", *version);
		return std::nullopt;
	}
}

std::optional<CE::AssetFileMetaData> CE::AssetFileMetaData::ReadMetaDataV0(std::istream& fromStream, uint32)
{
	// In earlier versions, every asset version was 0.
	// we did not save the metadata version yet.
	// So if the first version variable is 0,
	// we assume that metadata version is 0.
	//
	// Starting from metaDataVersion 1, we save two version
	// variables; the first one for the metadata version,
	// the second for the asset version.
	uint32 assetVersion = 0;

	std::optional<TypeId> assetClassTypeId = TryRead<TypeId>(fromStream);

	if (!assetClassTypeId.has_value())
	{
		LOG(LogAssets, Message, "Asset metadata was incomplete");
		return std::nullopt;
	}

	const MetaType* assetClass = MetaManager::Get().TryGetType(*assetClassTypeId);

	if (assetClass == nullptr)
	{
		LOG(LogAssets, Error, "Asset metadata invalid: There is no class with typeid of \"{}\"", *assetClassTypeId);
		return std::nullopt;
	}

	if (!assetClass->IsDerivedFrom<Asset>())
	{
		LOG(LogAssets, Error, "Asset metadata invalid: \"{}\" does not derive from Asset", assetClass->GetName());
		return std::nullopt;
	}

	std::optional<uint16> nameSize = TryRead<uint16>(fromStream);

	if (!nameSize.has_value())
	{
		LOG(LogAssets, Message, "Asset metadata was incomplete");
		return std::nullopt;
	}

	std::string name{};
	name.resize(*nameSize);
	if (fromStream.readsome(name.data(), *nameSize) != *nameSize)
	{
		LOG(LogAssets, Message, "Asset metadata was incomplete");
		return std::nullopt;
	}

	// Now that we know the name, we can give a proper warning message
	LOG(LogAssets, Error, "Asset {} needs to be resaved - MetaData of version 0 is deprecated. This asset cannot be loaded on a different compiler than the one it was saved with.", name);

	std::optional<bool> wasImportedFromFile = TryRead<bool>(fromStream);

	if (!wasImportedFromFile.has_value())
	{
		LOG(LogAssets, Message, "Asset metadata was incomplete");
		return std::nullopt;
	}

	if (!*wasImportedFromFile)
	{
		return AssetFileMetaData{ name, *assetClass, assetVersion };
	}

	std::optional<uint32> importerVersion = TryRead<uint32>(fromStream);

	if (!importerVersion.has_value())
	{
		LOG(LogAssets, Message, "Asset metadata was incomplete");
		return std::nullopt;
	}

	std::optional<uint16> pathSize = TryRead<uint16>(fromStream);

	if (!pathSize.has_value())
	{
		LOG(LogAssets, Message, "Asset metadata was incomplete");
		return std::nullopt;
	}

	std::string filePath{};
	filePath.resize(*pathSize);
	if (fromStream.readsome(filePath.data(), *pathSize) != *pathSize)
	{
		LOG(LogAssets, Message, "Asset metadata was incomplete");
		return std::nullopt;
	}

	return AssetFileMetaData{ name, *assetClass, assetVersion, ImporterInfo{ filePath, *importerVersion } };
}

std::optional<CE::AssetFileMetaData> CE::AssetFileMetaData::ReadMetaDataV1V2(std::istream& fromStream, uint32 version)
{
	BinaryGSONObject obj{};

	if (!obj.LoadFromBinary(fromStream))
	{
		LOG(LogAssets, Message, "Asset metadata was corrupted - GSON parsing failed");
		return std::nullopt;
	}

	const BinaryGSONMember* const savedAssetVersion = obj.TryGetGSONMember("av");
	const BinaryGSONMember* const savedClass = obj.TryGetGSONMember("tpId") ;
	const BinaryGSONMember* const savedAssetName = obj.TryGetGSONMember("nm");

	if (savedAssetVersion == nullptr
		|| savedClass == nullptr
		|| savedAssetName == nullptr)
	{
		LOG(LogAssets, Message, "Asset metadata was corrupted - Missing vital information");
		return std::nullopt;
	}

	uint32 assetVersion{};
	std::string assetClassName{};
	std::string assetName{};

	*savedAssetVersion >> assetVersion;
	*savedClass >> assetClassName;
	*savedAssetName >> assetName;

	const MetaType* assetClass = MetaManager::Get().TryGetType(assetClassName);

	if (assetClass == nullptr)
	{
		LOG(LogAssets, Message, "Failed to load asset {} - Class {} has been deleted or renamed.", assetName, assetClassName);
		return std::nullopt;
	}

	if (!assetClass->IsDerivedFrom<Asset>())
	{
		LOG(LogAssets, Message, "Failed to load asset {} - Class {} does not derive from Asset.", assetName, assetClassName);
		return std::nullopt;
	}

	const BinaryGSONMember* const savedImporterVersion = obj.TryGetGSONMember("iv");
	const BinaryGSONMember* const savedImportedFromFile = obj.TryGetGSONMember("iff");

	std::optional<ImporterInfo> importerInfo{};

	if (savedImporterVersion != nullptr
		|| savedImportedFromFile != nullptr)
	{
		if ((savedImporterVersion == nullptr)
			!= (savedImportedFromFile == nullptr))
		{
			LOG(LogAssets, Message, "Asset metadata was corrupted");
			return std::nullopt;
		}

		importerInfo.emplace();

		*savedImporterVersion >> importerInfo->mImporterVersion;

		std::string importedFromFile{};
		*savedImportedFromFile >> importedFromFile;;
		importerInfo->mImportedFile = importedFromFile;

		if (version >= 2)
		{
			const BinaryGSONMember* const savedWriteTime = obj.TryGetGSONMember("wt");
			const BinaryGSONMember* const savedEditsAfterImporting = obj.TryGetGSONMember("ed");

			if (savedWriteTime == nullptr
				|| savedEditsAfterImporting == nullptr)
			{
				LOG(LogAssets, Message, "Asset metadata was corrupted");
				return std::nullopt;
			}

			using TimeSinceEpoch = decltype(importerInfo->mImportedFromFileWriteTimeAtTimeOfImporting.time_since_epoch());
			using Count = decltype(importerInfo->mImportedFromFileWriteTimeAtTimeOfImporting.time_since_epoch().count());
			Count serialisedTimeSinceEpoch{};
			*savedWriteTime >> serialisedTimeSinceEpoch;
			const TimeSinceEpoch sinceEpoch{ serialisedTimeSinceEpoch };
			importerInfo->mImportedFromFileWriteTimeAtTimeOfImporting = decltype(importerInfo->mImportedFromFileWriteTimeAtTimeOfImporting){ sinceEpoch };

			*savedEditsAfterImporting >> importerInfo->mWereEditsMadeAfterImporting;
		}
	}

	AssetFileMetaData metaData{ std::move(assetName), *assetClass, assetVersion, std::move(importerInfo) };
	metaData.mMetaDataVersion = version;
	return metaData;
}

void CE::AssetFileMetaData::WriteMetaData(std::ostream& toStream) const
{
	toStream.write(reinterpret_cast<const char*>(&sMetaDataVersion), sizeof(sMetaDataVersion));

	BinaryGSONObject obj{};

	obj.AddGSONMember("av") << mAssetVersion;
	obj.AddGSONMember("tpId") << mClass.get().GetName();
	obj.AddGSONMember("nm") << mAssetName;

	if (mImporterInfo.has_value())
	{
		obj.AddGSONMember("iv") << mImporterInfo->mImporterVersion;
		obj.AddGSONMember("iff") << mImporterInfo->mImportedFile.string();
		obj.AddGSONMember("wt") << mImporterInfo->mImportedFromFileWriteTimeAtTimeOfImporting.time_since_epoch().count();
		obj.AddGSONMember("ed") << mImporterInfo->mWereEditsMadeAfterImporting;
	}

	obj.SaveToBinary(toStream);
}

