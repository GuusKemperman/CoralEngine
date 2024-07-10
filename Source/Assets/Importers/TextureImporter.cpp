#include "Precomp.h"
#include "Assets/Importers/TextureImporter.h"

#include "xsr/backends/common/stb_image.h"
#include "xsr/backends/common/stbi_image_write.h"

#include "Assets/Texture.h"
#include "Utilities/ClassVersion.h"
#include "Meta/MetaType.h"

std::optional<std::vector<Engine::ImportedAsset>> Engine::TextureImporter::Import(const std::filesystem::path& path) const
{
	int width, height, channels;
	stbi_set_flip_vertically_on_load(false);
	
	unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);

	if (pixels == nullptr)
	{
		LOG(LogAssets, Error, "Importing texture failed: stbi returned nullptr", path.string());
		return std::optional<std::vector<ImportedAsset>>{};
	}

	const MetaType* const myType = MetaManager::Get().TryGetType<TextureImporter>();
	ASSERT(myType != nullptr);

	std::optional<ImportedAsset> importedTexture = ImportFromMemory(path, 
		path.filename().replace_extension().string(), 
		GetClassVersion(*myType),
		{ reinterpret_cast<char*>(pixels), static_cast<size_t>(width) * static_cast<size_t>(height) * 4u }, 
		width, 
		height);

	stbi_image_free(pixels);

	if (importedTexture.has_value())
	{
		std::vector<ImportedAsset> returnValue{};
		returnValue.emplace_back(std::move(*importedTexture));
		return returnValue;
	}
	return std::optional<std::vector<ImportedAsset>>{};
}


std::optional<Engine::ImportedAsset> Engine::TextureImporter::ImportFromMemory(const std::filesystem::path& importedFromFile,
	const std::string& name,
	const uint32 importerVersion,
	const Span<const char> buffer,
	const uint32 width,
	const uint32 height)
{
	const uint32 totalNumOfPixels = width * height;

	if (width == 0
		|| height == 0)
	{
		LOG(LogAssets, Error, "Importing texture failed: invalid dimesnsions ({}x{})", width, height);
		return std::optional<ImportedAsset>{};
	}

	if (buffer.size() != totalNumOfPixels * 4)
	{
		LOG(LogAssets, Error, "Importing texture failed: wrong number of bytes, received {} but expected {} (RGBA) bytes",
			buffer.size(),
			totalNumOfPixels * 4);

		return std::optional<ImportedAsset>{};
	}

	const MetaType* const textureType = MetaManager::Get().TryGetType<Texture>();
	ASSERT(textureType != nullptr);

	std::string pngData{};
	auto func = [](void* context, void* data, int size)
		{
			static_cast<std::string*>(context)->append(static_cast<const char*>(data), size);
		};
	
	if (stbi_write_png_to_func(func, &pngData, static_cast<int>(width), static_cast<int>(height), 4, buffer.data(), width * 4) == 0)
	{
		LOG(LogAssets, Error, "Importing texture failed: could not convert data to png");
		return std::optional<ImportedAsset>{};
	}

	ImportedAsset texture{ name, *textureType, importedFromFile, importerVersion };
	texture.GetStream() << pngData;

	return texture;
}

Engine::MetaType Engine::TextureImporter::Reflect()
{
	MetaType type = MetaType{MetaType::T<TextureImporter>{}, "TextureImporter", MetaType::Base<Importer>{} };
	return type;
}
