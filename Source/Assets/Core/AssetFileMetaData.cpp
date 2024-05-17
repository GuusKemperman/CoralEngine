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

std::optional<CE::AssetFileMetaData> CE::AssetFileMetaData::ReadMetaData(std::istream& fromStream)
{
	uint32 version{};
	fromStream.read(reinterpret_cast<char*>(&version), sizeof(version));
	const std::streamsize amountRead = fromStream.gcount();

	if (amountRead != sizeof(version))
	{
		LOG(LogAssets, Message, "Asset metadata was empty");
		return std::nullopt;
	}

	switch(version)
	{
	case 1:
	case 2:
	case 3:
		return ReadMetaDataV1V2V3(fromStream, version);
	case 4:
	{
		try
		{
			cereal::BinaryInputArchive ar{ fromStream };
			uint32 sizeOfMetaData{};
			ar(sizeOfMetaData);
			return ReadMetaDataV4(ar);
		}
		catch ([[maybe_unused]] const std::exception& e)
		{
			LOG(LogAssets, Warning, "Asset metadata version {} could not be loaded - {}", version, e.what());
			return std::nullopt;
		}
	}
	default:
		LOG(LogAssets, Message, "Asset metadata version {} is not recognised and not supported", version);
		return std::nullopt;
	}
}

std::optional<CE::AssetFileMetaData> CE::AssetFileMetaData::ReadMetaData(const std::filesystem::path& fromFile)
{
	FILE* file{};
	const errno_t err = fopen_s(&file, fromFile.string().c_str(), "rb");

	if (file == nullptr)
	{
		LOG(LogAssets, Warning, "Could not open {} - {}", fromFile.string(), err);
		return std::nullopt;
	}

	static constexpr size_t maxSize = 256;
	char stackBuffer[maxSize];
	size_t numBytesRead = fread(stackBuffer, sizeof(stackBuffer[0]), maxSize, file);

	uint32 sizeOfMetaData{};
	uint32 metaDataVersion{};

	constexpr uint32 numBytesConsumedInThisFunction = sizeof(metaDataVersion) + sizeof(sizeOfMetaData);

	if (numBytesRead < numBytesConsumedInThisFunction)
	{
		LOG(LogAssets, Warning, "{} only contained {} bytes", fromFile.string(), numBytesRead);
		return std::nullopt;
	}

	metaDataVersion = *reinterpret_cast<uint32*>(stackBuffer);

	switch (metaDataVersion)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	{
		(void)fclose(file);

		// Older version, we completely discard everything we read so far and start over.
		std::ifstream fileStream{ fromFile, std::ifstream::binary };

		if (!fileStream.is_open())
		{
			LOG(LogAssets, Warning, "Could not open {} - {}", fromFile.string(), err);
			return std::nullopt;
		}

		return ReadMetaData(fileStream);
	}
	case 4:
	{
		sizeOfMetaData = *reinterpret_cast<uint32*>(stackBuffer + 4);

		const char* bufferToUse = stackBuffer;

		if (sizeOfMetaData > maxSize)
		{
			std::string heapBuffer{ stackBuffer, numBytesRead };
			heapBuffer.resize(sizeOfMetaData);
			bufferToUse = heapBuffer.data();

			// Read the remaining bytes
			numBytesRead += fread(heapBuffer.data() + numBytesRead, sizeof(heapBuffer[0]), sizeOfMetaData - numBytesRead, file);
		}

		(void)fclose(file);

		if (numBytesRead < sizeOfMetaData)
		{
			LOG(LogAssets, Warning, "{} only contained {} bytes, but expected {}", fromFile.string(), numBytesRead, sizeOfMetaData);
			return std::nullopt;
		}

		view_istream stream = { std::string_view{ bufferToUse + numBytesConsumedInThisFunction, sizeOfMetaData - numBytesConsumedInThisFunction}};
		cereal::BinaryInputArchive ar{ stream };
		return ReadMetaDataV4(ar);
	}
	default:
		(void)fclose(file);
		LOG(LogAssets, Message, "Asset metadata version {} is not recognised and not supported", metaDataVersion);
		return std::nullopt;
	}
}

std::optional<CE::AssetFileMetaData> CE::AssetFileMetaData::ReadMetaDataV1V2V3(std::istream& fromStream, uint32 version)
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
			const BinaryGSONMember* const savedEditsAfterImporting = obj.TryGetGSONMember("ed");

			if (savedEditsAfterImporting == nullptr)
			{
				LOG(LogAssets, Message, "Asset metadata was corrupted");
				return std::nullopt;
			}

			*savedEditsAfterImporting >> importerInfo->mWereEditsMadeAfterImporting;
		}
	}

	AssetFileMetaData metaData{ std::move(assetName), *assetClass, assetVersion, std::move(importerInfo) };
	metaData.mMetaDataVersion = version;
	return metaData;
}

std::optional<CE::AssetFileMetaData> CE::AssetFileMetaData::ReadMetaDataV4(cereal::BinaryInputArchive& fromArchive)
{
	uint32 assetVersion{};
	Name::HashType hashedAssetClassName{};
	std::string assetName{};
	bool wasImported{};

	try
	{
		fromArchive(assetVersion, hashedAssetClassName, assetName, wasImported);

		const MetaType* assetClass = MetaManager::Get().TryGetType(Name{ hashedAssetClassName });

		if (assetClass == nullptr)
		{
			LOG(LogAssets, Message, "Failed to load asset {} - Class {} has been deleted or renamed.", assetName, hashedAssetClassName);
			return std::nullopt;
		}

		if (!assetClass->IsDerivedFrom<Asset>())
		{
			LOG(LogAssets, Message, "Failed to load asset {} - Class {} does not derive from Asset.", assetName, assetClass->GetName());
			return std::nullopt;
		}

		std::optional<ImporterInfo> importerInfo{};

		if (wasImported)
		{
			importerInfo.emplace();
			std::string importedFromFile{};
			fromArchive(importedFromFile, importerInfo->mImporterVersion, importerInfo->mWereEditsMadeAfterImporting);
			importerInfo->mImportedFile = { std::move(importedFromFile) };
		}

		std::optional<AssetFileMetaData> metaData{};
		metaData.emplace(std::move(assetName), *assetClass, assetVersion, std::move(importerInfo));
		metaData->mMetaDataVersion = 4;
		return metaData;
	}
	catch (const std::exception& e)
	{
		LOG(LogAssets, Warning, "Failed to load metadata - {}", e.what());
		return std::nullopt;
	}
}

void CE::AssetFileMetaData::WriteMetaData(std::ostream& toStream) const
{
	std::ostringstream stringStream{};

	{
		cereal::BinaryOutputArchive ar{ stringStream };

		ar(mAssetVersion, Name::HashString(mClass.get().GetName()), mAssetName, mImporterInfo.has_value());

		if (mImporterInfo.has_value())
		{
			ar(mImporterInfo->mImportedFile.string(), mImporterInfo->mImporterVersion, mImporterInfo->mWereEditsMadeAfterImporting);
		}
	}

	const std::string asString = stringStream.str();
	const uint32 size = static_cast<uint32>(asString.size()) + sizeof(sMetaDataVersion) + sizeof(size);
	toStream.write(reinterpret_cast<const char*>(&sMetaDataVersion), sizeof(sMetaDataVersion));
	toStream.write(reinterpret_cast<const char*>(&size), sizeof(size));
	toStream.write(asString.data(), asString.size());
}

