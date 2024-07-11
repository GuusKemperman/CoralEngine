#include "Precomp.h"
#include "Assets/Importers/AudioImporter.h"

#include "Assets/Sound.h"
#include "Meta/MetaManager.h"
#include "Utilities/StringFunctions.h"

CE::Importer::ImportResult CE::AudioImporter::Import(const std::filesystem::path& path) const
{
	std::ifstream stream{ path, std::ifstream::binary };

	if (!stream.is_open())
	{
		return std::nullopt;
	}

	std::string fileContents = StringFunctions::StreamToString(stream);

	std::vector<ImportedAsset> returnValue{};
	ImportedAsset& asset = returnValue.emplace_back(path.filename().replace_extension().string(), MetaManager::Get().GetType<Sound>(), path, 0);
	asset.GetStream().write(fileContents.data(), fileContents.size());

	return returnValue;
}

CE::MetaType CE::AudioImporter::Reflect()
{
	return MetaType{ MetaType::T<AudioImporter>{}, "AudioImporter", MetaType::Base<Importer>{} };
}
