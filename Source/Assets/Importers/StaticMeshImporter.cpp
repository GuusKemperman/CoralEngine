#include "Precomp.h"
#include "Assets/Importers/StaticMeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/***REMOVED***ne.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

#include "Assets/StaticMesh.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "stb_image/stb_image.h"
#include "Assets/Importers/MaterialImporter.h"
#include "Assets/Importers/TextureImporter.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Assets/Importers/PrefabImporter.h"
#include "Components/NameComponent.h"
#include "Components/TransformComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/AssetManager.h"
#include "Utilities/ClassVersion.h"
#include "Meta/MetaManager.h"

static std::string GetTexName(const std::filesystem::path& modelPath, const int index)
{
	return modelPath.filename().replace_extension().string().append("_tex").append(std::to_string(index));
}

static std::string GetMeshName(const std::filesystem::path& modelPath, const char* name)
{
	return modelPath.filename().replace_extension().string().append("_").append(name);
}

static std::string Get***REMOVED***neName(const std::filesystem::path& modelPath)
{
	return modelPath.filename().replace_extension().string().append("_").append("Prefab");
}

static std::string GetMaterialName(const std::filesystem::path& modelPath, const char* name, const int index)
{
	return (*name == *"") ? "M_" + modelPath.filename().string() + *"_Unnamed_Material_" + std::to_string(index) : name;
}

static int GetIndexFromAssimpTextureName(const char* name)
{
	return atoi(std::string(name).erase(0, 1).c_str());
}

std::optional<std::vector<Engine::ImportedAsset>> Engine::StaticMeshImporter::Import(const std::filesystem::path& file) const
{
    Assimp::Importer importer{};
    const ai***REMOVED***ne* ***REMOVED***ne = importer.ReadFile(file.string(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_RemoveRedundantMaterials | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);

	if (***REMOVED***ne == nullptr)
	{
		LOG(LogAssets, Error, "Loading of file {} failed: assimp returned null ***REMOVED***ne", file.string());
		return std::optional<std::vector<ImportedAsset>>{};
	}

	const MetaType* const myType = MetaManager::Get().TryGetType<StaticMeshImporter>();
	ASSERT(myType != nullptr);
	
	const uint32 myVersion = GetClassVersion(*myType);

	std::vector<ImportedAsset> returnValue{};

	// Not really loaded in, but materials hold a shared ptr to textures
	std::vector<std::shared_ptr<const Texture>> textures{};

	// We set it to false initially. If we encounter errors, we continue importing for a bit (but set this to false), 
	// just so we can log more errors and inform the user of any other issues.
	bool anyErrors = false;

	for (uint32 i = 0; i < ***REMOVED***ne->mNumTextures; i++)
	{
		const aiTexture& aiTex = ****REMOVED***ne->mTextures[i];

		const std::string textureName = GetTexName(file, i);

		if (AssetManager::Get().TryGetWeakAsset<Texture>(textureName).has_value())
		{
			LOG(LogAssets, Message, "Texture {} will not be imported, as there is already a texture with this name", textureName);
			continue;
		}

		std::optional<ImportedAsset> importedTexture{};

		const int size = aiTex.mHeight != 0 ? aiTex.mWidth * aiTex.mHeight : aiTex.mWidth;

		int width{}, height{}, channels{};

		const Engine::Span<char> pixels = { 
			reinterpret_cast<char*>(stbi_load_from_memory(reinterpret_cast<const unsigned char*>(aiTex.pcData), size, &width, &height, &channels, 4)),
			static_cast<unsigned int>(width * height * 4)};

		importedTexture = TextureImporter::ImportFromMemory(file,
			textureName,
			myVersion,
			pixels, 
			static_cast<uint32>(width),
			static_cast<uint32>(height));

		if (importedTexture.has_value())
		{
			returnValue.emplace_back(std::move(*importedTexture));
		}
		else
		{
			LOG(LogAssets, Error, "Failed to import {}", textureName);
			anyErrors = true;
		}

		textures.emplace_back(std::make_shared<const Texture>(textureName));
	}

	for (uint32 i = 0; i < ***REMOVED***ne->mNumMaterials; i++)
	{
		const aiMaterial& aiMat = ****REMOVED***ne->mMaterials[i];

		if (AssetManager::Get().TryGetWeakAsset<Material>(aiMat.GetName().C_Str()).has_value())
		{
			LOG(LogAssets, Message, "Material {} will not be imported, as there is already a material with this name", aiMat.GetName().C_Str());
			continue;
		}

		Material engineMat{ GetMaterialName(file, aiMat.GetName().C_Str(), i) };

		aiGetMaterialColor(&aiMat, AI_MATKEY_BASE_COLOR, reinterpret_cast<aiColor4D*>(&engineMat.mBaseColorFactor));
		
		if (aiColor4D tmpEmmisveFactor{}; aiGetMaterialColor(&aiMat, AI_MATKEY_COLOR_EMISSIVE, &tmpEmmisveFactor) == aiReturn_SUCCESS) // glm::vec3 mEmissiveFactor{};
		{
			engineMat.mEmissiveFactor = { tmpEmmisveFactor.r, tmpEmmisveFactor.g, tmpEmmisveFactor.b };
		}
			
		aiGetMaterialFloat(&aiMat, AI_MATKEY_METALLIC_FACTOR, &engineMat.mMetallicFactor); // float mMetallicFactor = 1.0f;
		aiGetMaterialFloat(&aiMat, AI_MATKEY_ROUGHNESS_FACTOR, &engineMat.mRoughnessFactor);	// float mRoughnessFactor = 1.0f;
		aiGetMaterialFloat(&aiMat, AI_MATKEY_GLTF_ALPHACUTOFF, &engineMat.mAlphaCutoff);	// float mAlphaCutoff = .5f;
		aiGetMaterialFloat(&aiMat, AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), &engineMat.mNormalScale); // float mNormalScale = 1.0f;
		aiGetMaterialFloat(&aiMat, AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_AMBIENT_OCCLUSION, 0), &engineMat.mOcclusionStrength); // float mOcclusionStrength;

		if (int tmpDoubleSided{}; aiGetMaterialInteger(&aiMat, AI_MATKEY_TWOSIDED, &tmpDoubleSided) == aiReturn_SUCCESS) // bool mDoubleSided{};
		{
			engineMat.mDoubleSided = tmpDoubleSided;
		}

		aiString textureName{};
		if (aiGetMaterialTexture(&aiMat, aiTextureType_BASE_COLOR, 0, &textureName) == aiReturn_SUCCESS) // std::shared_ptr<const Texture> mBaseColorTexture{};
		{
			engineMat.mBaseColorTexture = textures[GetIndexFromAssimpTextureName(textureName.C_Str())];
		}

		if (aiGetMaterialTexture(&aiMat, aiTextureType_NORMALS, 0, &textureName) == aiReturn_SUCCESS) // std::shared_ptr<const Texture> mNormalTexture{};
		{
			engineMat.mNormalTexture = textures[GetIndexFromAssimpTextureName(textureName.C_Str())];
		}

		if (aiGetMaterialTexture(&aiMat, aiTextureType_AMBIENT_OCCLUSION, 0, &textureName) == aiReturn_SUCCESS) // std::shared_ptr<const Texture> mOcclusionTexture{};
		{
			engineMat.mOcclusionTexture = textures[GetIndexFromAssimpTextureName(textureName.C_Str())];
		}

		if (aiGetMaterialTexture(&aiMat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &textureName) == aiReturn_SUCCESS) // std::shared_ptr<const Texture> mMetallicRoughnessTexture{};
		{
			engineMat.mMetallicRoughnessTexture = textures[GetIndexFromAssimpTextureName(textureName.C_Str())];
		}

		if (aiGetMaterialTexture(&aiMat, aiTextureType_EMISSIVE, 0, &textureName) == aiReturn_SUCCESS) // std::shared_ptr<const Texture> mEmissiveTexture{};
		{
			engineMat.mEmissiveTexture = textures[GetIndexFromAssimpTextureName(textureName.C_Str())];
		}

		returnValue.emplace_back(MaterialImporter::Import(file, myVersion, engineMat));
	}

	for (uint32 i = 0; i < ***REMOVED***ne->mNumMeshes; i++)
	{
		const aiMesh& mesh = ****REMOVED***ne->mMeshes[i];

		const std::string meshName = GetMeshName(file, mesh.mName.C_Str());

		const Span<const glm::vec3> positions = { reinterpret_cast<const glm::vec3*>(mesh.mVertices), mesh.mNumVertices };
		std::optional<std::vector<uint32>> indices{};
		std::optional<Span<const glm::vec3>> normals{};
		std::optional<std::vector<glm::vec2>> textureCoordinates{};
		std::optional<std::vector<glm::vec3>> tangents{};

		if (mesh.HasFaces())
		{
			indices = std::vector<uint32>(mesh.mNumFaces * 3);
			size_t insertAt = 0;
			for (uint32 faceI = 0; faceI < mesh.mNumFaces; faceI++)
			{
				const aiFace& face = mesh.mFaces[faceI];
				for (uint32 faceJ = 0; faceJ < face.mNumIndices; faceJ++, insertAt++)
				{
					(*indices)[insertAt] = face.mIndices[faceJ];
				}
			}
		}

		if (mesh.HasNormals())
		{			
			normals = { reinterpret_cast<const glm::vec3*>(mesh.mNormals), mesh.mNumVertices};
		}

		if (mesh.HasTextureCoords(0))
		{
			textureCoordinates = std::vector<glm::vec2>(mesh.mNumVertices);
			for (uint32 v = 0; v < mesh.mNumVertices; v++)
			{
				(*textureCoordinates)[v] = { mesh.mTextureCoords[0][v].x,  mesh.mTextureCoords[0][v].y };
			}
		}

		if (mesh.HasTangentsAndBitangents())
		{
			tangents = std::vector<glm::vec3>( reinterpret_cast<const glm::vec3*>(mesh.mTangents), reinterpret_cast<const glm::vec3*>(mesh.mTangents) + mesh.mNumVertices );
		}

		std::optional<ImportedAsset> importedMesh = ImportFromMemory(file, meshName, myVersion, positions, indices, normals, tangents, textureCoordinates);

		if (importedMesh.has_value())
		{
			returnValue.emplace_back(std::move(*importedMesh));
		}
		else
		{
			LOG(LogAssets, Error, "Loading of mesh {} failed: converting assimp results to engine representation failed", meshName);
			anyErrors = true;
		}
	}

	if (anyErrors)
	{
		return std::optional<std::vector<ImportedAsset>>{};
	}

	World world{ false };
	Registry& reg = world.GetRegistry();

	std::function<entt::entity(const aiNode&, std::optional<entt::entity>)> makePrefab = [&](const aiNode& node, std::optional<entt::entity> parent)
		{
			const entt::entity entity = reg.Create();
			reg.AddComponent<NameComponent>(entity, node.mName.C_Str());
			TransformComponent& transform = reg.AddComponent<TransformComponent>(entity);
			
			if (parent.has_value())
			{
				transform.SetParent(&reg.Get<TransformComponent>(*parent));
			}

			transform.SetLocalMatrix(glm::transpose(reinterpret_cast<const glm::mat4&>(node.mTransformation)));

			for (size_t i = 0; i < node.mNumMeshes; i++)
			{
				entt::entity meshHolder{};

				if (i == 0)
				{
					meshHolder = entity;
				}
				else
				{
					meshHolder = reg.Create();
					reg.AddComponent<TransformComponent>(meshHolder).SetParent(&transform);
				}

				StaticMeshComponent& meshComponent = reg.AddComponent<StaticMeshComponent>(entity);

				std::shared_ptr<StaticMesh> mesh = std::make_shared<StaticMesh>(GetMeshName(file, ***REMOVED***ne->mMeshes[node.mMeshes[i]]->mName.C_Str()));
				meshComponent.mStaticMesh = std::move(mesh);
				
				std::shared_ptr<Material> mat = std::make_shared<Material>(GetMaterialName(file, ***REMOVED***ne->mMaterials[***REMOVED***ne->mMeshes[node.mMeshes[i]]->mMaterialIndex]->GetName().C_Str(), ***REMOVED***ne->mMeshes[node.mMeshes[i]]->mMaterialIndex));
				meshComponent.mMaterial = std::move(mat);
			}

			for (size_t i = 0; i < node.mNumChildren; i++)
			{
				const aiNode& child = *node.mChildren[i];

				makePrefab(child, entity);
			}

			return entity;
		};

	const entt::entity prefabEntity = makePrefab(****REMOVED***ne->mRootNode, {});

	std::optional<ImportedAsset> importedPrefab = PrefabImporter::MakePrefabFromEntity(file, Get***REMOVED***neName(file), myVersion, world, prefabEntity);

	if (importedPrefab.has_value())
	{
		returnValue.emplace_back(std::move(*importedPrefab));
		return returnValue;
	}
	
	LOG(LogError, Error, "Importing {} failed; failed to make a valid prefab out of the ai ***REMOVED***ne", file.string());
	return std::optional<std::vector<ImportedAsset>>{};
}

std::optional<Engine::ImportedAsset> Engine::StaticMeshImporter::ImportFromMemory(const std::filesystem::path& importedFromFile,
    const std::string& name, 
    const uint32 importerVersion, 
    Span<const glm::vec3> positions, 
	std::optional<std::variant<Span<const uint16>, Span<const uint32>>> indices,
    std::optional<Span<const glm::vec3>> normals,
	std::optional<Span<const glm::vec3>> tangents,
    std::optional<Span<const glm::vec2>> textureCoordinates)
{
	const MetaType* const staticMeshType = MetaManager::Get().TryGetType<StaticMesh>();
	ASSERT(staticMeshType != nullptr);

    ImportedAsset importedMesh{ name, *staticMeshType, importedFromFile, importerVersion};
    if (StaticMesh::OnSave(importedMesh, positions, indices, normals, tangents, textureCoordinates))
    {
        return importedMesh;
    }
    return std::optional<ImportedAsset>();
}

Engine::MetaType Engine::StaticMeshImporter::Reflect()
{
	MetaType type = MetaType{ MetaType::T<StaticMeshImporter>{}, "StaticMeshImporter", MetaType::Base<Importer>{} };

	// Added tangents
	SetClassVersion(type, 1);

	return type;
}
