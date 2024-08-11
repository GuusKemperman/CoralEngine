#include "Precomp.h"
#include "Assets/Importers/ModelImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
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

namespace
{
	template<typename T>
	struct DummyAsset
	{
		DummyAsset(std::string_view name) :
			mDummyAsset(std::make_unique<CE::Internal::AssetInternal>(CE::AssetFileMetaData{ name, CE::MetaManager::Get().GetType<T>() }, std::filesystem::path{})),
			mHandle(mDummyAsset.get())
		{}
		std::unique_ptr<CE::Internal::AssetInternal> mDummyAsset{};
		CE::AssetHandle<T> mHandle{};
	};

	std::string RemoveExtension(const auto& str)
	{
		return std::filesystem::path{ str }.filename().replace_extension().string();
	}

	std::string GetEmbedTexName(const std::filesystem::path& modelPath, const int index)
	{
		return CE::Format("T_{}_{}", RemoveExtension(modelPath), index);
	}

	std::string GetTexName(std::string_view name)
	{
		return CE::Format("T_{}", RemoveExtension(name));
	}

	std::string GetMeshName(std::string_view name)
	{
		return CE::Format("SM_{}", RemoveExtension(name));
	}

	std::string GetSceneName(const std::filesystem::path& modelPath)
	{
		return CE::Format("PF_{}", RemoveExtension(modelPath));
	}

	std::string GetMaterialName(const std::filesystem::path& modelPath, std::string_view name, const int index)
	{
		if (name.empty())
		{
			return CE::Format("MT_{}_{}", RemoveExtension(modelPath), index);
		}
		return CE::Format("MT_{}", name);
	}

	int GetIndexFromAssimpTextureName(const char* name)
	{
		return atoi(std::string(name).erase(0, 1).c_str());
	}
}

std::optional<std::vector<CE::ImportedAsset>> CE::ModelImporter::Import(const std::filesystem::path& file) const
{
    Assimp::Importer importer{};
    const aiScene* scene = importer.ReadFile(file.string(), aiProcess_LimitBoneWeights | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_RemoveRedundantMaterials | aiProcess_JoinIdenticalVertices | aiProcess_ConvertToLeftHanded);
	if (scene == nullptr)
	{
		LOG(LogAssets, Error, "Loading of file {} failed: assimp returned null scene - {}", file.string(), importer.GetErrorString());
		return Importer::ImportResult{};
	}

	const MetaType* const myType = MetaManager::Get().TryGetType<ModelImporter>();
	ASSERT(myType != nullptr);
	
	const uint32 myVersion = GetClassVersion(*myType);

	std::vector<ImportedAsset> returnValue{};

	// Not really loaded in, but materials hold a shared ptr to textures
	std::vector<DummyAsset<Texture>> textures{};

	// We set it to false initially. If we encounter errors, we continue importing for a bit (but set this to false), 
	// just so we can log more errors and inform the user of any other issues.
	bool anyErrors = false;

	for (uint32 i = 0; i < scene->mNumTextures; i++)
	{
		const aiTexture& aiTex = *scene->mTextures[i];

		const std::string textureName = GetEmbedTexName(file, i);

		std::optional<ImportedAsset> importedTexture{};

		const int size = aiTex.mHeight != 0 ? aiTex.mWidth * aiTex.mHeight : aiTex.mWidth;

		int width{}, height{}, channels{};

		const std::span<char> pixels = { 
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

		textures.emplace_back(textureName);
	}

	for (uint32 i = 0; i < scene->mNumMaterials; i++)
	{
		const aiMaterial& aiMat = *scene->mMaterials[i];

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

		auto getTexture = [&](aiTextureType aiType, int index) -> CE::AssetHandle<Texture>
			{
				if (aiGetMaterialTexture(&aiMat, aiType, index, &textureName) == aiReturn_SUCCESS)
				{
					if (textureName.C_Str()[0] == '*')
					{
						return textures[GetIndexFromAssimpTextureName(textureName.C_Str())].mHandle;
					}
					return textures.emplace_back(GetTexName(textureName.C_Str())).mHandle;
				}
				return nullptr;
			};

		engineMat.mBaseColorTexture = getTexture(aiTextureType_BASE_COLOR, 0);
		engineMat.mNormalTexture = getTexture(aiTextureType_NORMALS, 0);
		engineMat.mOcclusionTexture = getTexture(aiTextureType_LIGHTMAP, 0);
		engineMat.mMetallicRoughnessTexture = getTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE);
		engineMat.mEmissiveTexture = getTexture(aiTextureType_EMISSIVE, 0);

		returnValue.emplace_back(MaterialImporter::Import(file, myVersion, engineMat));
	}

	for (uint32 i = 0; i < scene->mNumMeshes; i++)
	{
		const aiMesh& mesh = *scene->mMeshes[i];

		const std::string meshName = GetMeshName(mesh.mName.C_Str());

		const std::span<const glm::vec3> positions = { reinterpret_cast<const glm::vec3*>(mesh.mVertices), mesh.mNumVertices };
		std::vector<uint32> indices{};
		std::span<const glm::vec3> normals{};
		std::vector<glm::vec2> textureCoordinates{};
		std::span<const glm::vec3> tangents{};

		if (mesh.HasFaces())
		{
			indices.resize(mesh.mNumFaces * 3);
			size_t insertAt = 0;
			for (uint32 faceI = 0; faceI < mesh.mNumFaces; faceI++)
			{
				const aiFace& face = mesh.mFaces[faceI];
				for (uint32 faceJ = 0; faceJ < face.mNumIndices; faceJ++, insertAt++)
				{
					indices[insertAt] = face.mIndices[faceJ];
				}
			}
		}

		if (mesh.HasNormals())
		{			
			normals = { reinterpret_cast<const glm::vec3*>(mesh.mNormals), mesh.mNumVertices};
		}

		if (mesh.HasTextureCoords(0))
		{
			textureCoordinates.resize(mesh.mNumVertices);
			for (uint32 v = 0; v < mesh.mNumVertices; v++)
			{
				textureCoordinates[v] = { mesh.mTextureCoords[0][v].x,  mesh.mTextureCoords[0][v].y };
			}
		}

		if (mesh.HasTangentsAndBitangents())
		{
			tangents = std::span<const glm::vec3>{ reinterpret_cast<const glm::vec3*>(mesh.mTangents), mesh.mNumVertices };
		}

		std::optional<ImportedAsset> importedMesh{}; 
		importedMesh = ImportFromMemory(file, meshName, myVersion, positions, indices, normals, tangents, textureCoordinates);

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
		return Importer::ImportResult{};
	}

	std::vector<DummyAsset<StaticMesh>> dummyStaticMeshes{};
	std::vector<DummyAsset<Material>> dummyMaterials{};

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


				AssetHandle<Material> mat = dummyMaterials.emplace_back(GetMaterialName(file, 
					scene->mMaterials[scene->mMeshes[node.mMeshes[i]]->mMaterialIndex]->GetName().C_Str(), 
					scene->mMeshes[node.mMeshes[i]]->mMaterialIndex)).mHandle;


				StaticMeshComponent& meshComponent = reg.AddComponent<StaticMeshComponent>(meshHolder);

				meshComponent.mStaticMesh = dummyStaticMeshes.emplace_back(GetMeshName(scene->mMeshes[node.mMeshes[i]]->mName.C_Str())).mHandle;
				meshComponent.mMaterial = std::move(mat);
			}

			for (size_t i = 0; i < node.mNumChildren; i++)
			{
				const aiNode& child = *node.mChildren[i];
				makePrefab(child, entity);
			}

			return entity;
		};

	scene->mRootNode->mName = file.filename().replace_extension().string();
	const entt::entity prefabEntity = makePrefab(*scene->mRootNode, {});

	std::optional importedPrefab = PrefabImporter::MakePrefabFromEntity(file, GetSceneName(file), myVersion, world, prefabEntity);

	if (importedPrefab.has_value())
	{
		returnValue.emplace_back(std::move(*importedPrefab));
		return returnValue;
	}
	
	LOG(LogError, Error, "Importing {} failed; failed to make a valid prefab out of the ai scene", file.string());
	return Importer::ImportResult{};
}

std::optional<CE::ImportedAsset> CE::ModelImporter::ImportFromMemory(const std::filesystem::path& importedFromFile,
	const std::string& name, uint32 importerVersion, std::span<const glm::vec3> positions, std::span<const uint32> indices,
	std::span<const glm::vec3> normals, std::span<const glm::vec3> tangents, std::span<const glm::vec2> textureCoordinates)
{
	const MetaType& staticMeshType = MetaManager::Get().GetType<StaticMesh>();

	ImportedAsset importedMesh{ name, staticMeshType, importedFromFile, importerVersion };
	if (Internal::SaveStaticMesh(importedMesh, positions, indices, normals, tangents, textureCoordinates))
	{
		return importedMesh;
	}
	return std::nullopt;
}

CE::MetaType CE::ModelImporter::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ModelImporter>{}, "ModelImporter", MetaType::Base<Importer>{} };

	SetClassVersion(type, 2);

	return type;
}
