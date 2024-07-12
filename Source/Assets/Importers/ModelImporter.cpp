#include "Precomp.h"
#include "Assets/Importers/ModelImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

#include "Assets/StaticMesh.h"
#include "Assets/SkinnedMesh.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/Animation/Animation.h"
#include "Assets/Animation/BoneInfo.h"
#include "Assets/Animation/Bone.h"
#include "stb_image/stb_image.h"
#include "Assets/Importers/MaterialImporter.h"
#include "Assets/Importers/TextureImporter.h"
#include "Assets/Importers/AnimationImporter.h"
#include "World/World.h"
#include "World/Registry.h"
#include "Assets/Importers/PrefabImporter.h"
#include "Components/NameComponent.h"
#include "Components/TransformComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Core/AssetManager.h"
#include "Utilities/ClassVersion.h"
#include "Meta/MetaManager.h"

static std::string GetEmbedTexName(const std::filesystem::path& modelPath, const int index)
{
	return modelPath.filename().replace_extension().string().append("_tex").append(std::to_string(index));
}

static std::string GetTexName(const char* name)
{
	return std::filesystem::path(name).filename().replace_extension().string();
}

static std::string GetMeshName(const std::filesystem::path& modelPath, const char* name)
{
	return modelPath.filename().replace_extension().string().append("_").append(name);
}

static std::string GetSceneName(const std::filesystem::path& modelPath)
{
	return modelPath.filename().replace_extension().string().append("_").append("Prefab");
}

static std::string GetMaterialName(const std::filesystem::path& modelPath, const char* name, const int index)
{
	return (*name == *"") ? "M_" + modelPath.filename().string() + *"_Unnamed_Material_" + std::to_string(index) : name;
}

static std::string GetAnimName(const std::filesystem::path& modelPath, const char* name)
{
	return modelPath.filename().replace_extension().string().append("_").append(name).append("_").append("Animation");
}

static int GetIndexFromAssimpTextureName(const char* name)
{
	return atoi(std::string(name).erase(0, 1).c_str());
}

static inline glm::vec3 GetGLMVec(const aiVector3D& vec) 
{ 
	return glm::vec3(vec.x, vec.y, vec.z); 
}

static inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
{
	return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}

void ReadHierarchyForAnimRecursive(const aiNode& node, CE::AnimNode& dest)
{
	dest.mName = node.mName.C_Str();
	dest.mTransform = glm::transpose(reinterpret_cast<const glm::mat4x4&>(node.mTransformation));

	for (size_t j = 0; j < node.mNumChildren; j++)
	{
		CE::AnimNode newNode;
		ReadHierarchyForAnimRecursive(*node.mChildren[j], newNode);
		dest.mChildren.push_back(newNode);
	}
}

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

		const CE::Span<char> pixels = { 
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

		const std::string meshName = GetMeshName(file, mesh.mName.C_Str());

		const Span<const glm::vec3> positions = { reinterpret_cast<const glm::vec3*>(mesh.mVertices), mesh.mNumVertices };
		std::optional<std::vector<uint32>> indices{};
		std::optional<Span<const glm::vec3>> normals{};
		std::optional<std::vector<glm::vec2>> textureCoordinates{};
		std::optional<std::vector<glm::vec3>> tangents{};
		
		std::optional<std::vector<glm::ivec4>> boneIds{};
		std::optional<std::vector<glm::vec4>> boneWeights{};
		std::optional<std::unordered_map<std::string, BoneInfo>> boneMap{};
		int boneCounter = 0;

		bool isSkinnedMesh = false;

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

		// Load bone data
		if (mesh.HasBones())
		{
			boneIds = std::vector<glm::ivec4>(mesh.mNumVertices, glm::ivec4(-1));
			boneWeights = std::vector<glm::vec4>(mesh.mNumVertices, glm::vec4(0.0f));
			boneMap = std::unordered_map<std::string, BoneInfo>{};

			for (unsigned int boneIndex = 0; boneIndex < mesh.mNumBones; boneIndex++)
			{
				int boneId = -1;
				std::string boneName = mesh.mBones[boneIndex]->mName.C_Str();
				
				if (boneMap->find(boneName) == boneMap->end())
				{
					BoneInfo newBoneInfo{};
					newBoneInfo.mId = boneCounter;
					newBoneInfo.mOffset = glm::transpose(reinterpret_cast<const glm::mat4&>(mesh.mBones[boneIndex]->mOffsetMatrix));
					(*boneMap)[boneName] = newBoneInfo;
					boneId = boneCounter;
					boneCounter++;
				}
				else 
				{
					boneId = (*boneMap)[boneName].mId;				
				}
				
				if (boneId == -1)
				{
					LOG(LogAssets, Error, "Loading of animated mesh {} in file {} failed", mesh.mName.C_Str(), file.string());
					anyErrors = true;
					break;
				}

				aiVertexWeight* weights = mesh.mBones[boneIndex]->mWeights;
				int numWeights = mesh.mBones[boneIndex]->mNumWeights;
				
				for (int weightIndex = 0; weightIndex < numWeights; weightIndex++)
				{
					unsigned int vertexId = weights[weightIndex].mVertexId;
					float weight = weights[weightIndex].mWeight;
					if (vertexId > mesh.mNumVertices)
					{
						LOG(LogAssets, Error, "Loading of animated mesh {} in file {} failed", mesh.mName.C_Str(), file.string());
						anyErrors = true;
						break;
					}

					glm::vec4* vertexBoneWeights = &(*boneWeights)[vertexId];
					glm::ivec4* vertexBoneIds = &(*boneIds)[vertexId];
					for (int j = 0; j < MAX_BONE_INFLUENCE; j++)
					{
						if ((*vertexBoneIds)[j] < 0)
						{
							(*vertexBoneWeights)[j] = weight;
							(*vertexBoneIds)[j] = boneId;
							break;
						}
					}
				}
			}
			isSkinnedMesh = true;
		}

		std::optional<ImportedAsset> importedMesh{}; 

		if (isSkinnedMesh)
		{
			importedMesh = ImportFromMemory(file, meshName, myVersion, positions, indices, normals, tangents, textureCoordinates, boneIds, boneWeights, boneMap);
		} 
		else
		{
			importedMesh = ImportFromMemory(file, meshName, myVersion, positions, indices, normals, tangents, textureCoordinates);
		}

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

	for (uint32 animIndex = 0; animIndex < scene->mNumAnimations; animIndex++)
	{
		const aiAnimation& aiAnim = *scene->mAnimations[animIndex];
		
		if (aiAnim.mNumChannels <= 0 )
		{
			continue;
		}
		
		Animation engineAnim{ GetAnimName(file, aiAnim.mName.C_Str()) };
		engineAnim.mDuration = static_cast<float>(aiAnim.mDuration);
		engineAnim.mTickPerSecond = static_cast<float>(aiAnim.mTicksPerSecond);
		
		engineAnim.mBones = std::vector<Bone>();
		
		ReadHierarchyForAnimRecursive(*scene->mRootNode, engineAnim.mRootNode);

		int size = aiAnim.mNumChannels;
		for (int channelIndex = 0; channelIndex < size; channelIndex++)
		{
			auto channel = aiAnim.mChannels[channelIndex];

			// Create bone
			AnimData animData{};
			animData.mPositions.resize(channel->mNumPositionKeys);
			animData.mRotations.resize(channel->mNumRotationKeys);
			animData.mScales.resize(channel->mNumScalingKeys);

			unsigned int numPositions = channel->mNumPositionKeys;
			unsigned int numRotations = channel->mNumRotationKeys;
			unsigned int numScalings = channel->mNumScalingKeys;

			for (unsigned int positionIndex = 0; positionIndex < numPositions; positionIndex++)
			{
				KeyPosition data{};
				data.position = GetGLMVec(channel->mPositionKeys[positionIndex].mValue);
				data.timeStamp = static_cast<float>(channel->mPositionKeys[positionIndex].mTime);
				animData.mPositions[positionIndex] = data;
			}

			for (unsigned int rotationIndex = 0; rotationIndex < numRotations; rotationIndex++)
			{
				KeyRotation data{};
				data.orientation = GetGLMQuat(channel->mRotationKeys[rotationIndex].mValue);
				data.timeStamp = static_cast<float>(channel->mRotationKeys[rotationIndex].mTime);
				animData.mRotations[rotationIndex] = data;
			}

			for (unsigned int scaleIndex = 0; scaleIndex < numScalings; scaleIndex++)
			{
				KeyScale data{};
				data.scale = GetGLMVec(channel->mScalingKeys[scaleIndex].mValue);
				data.timeStamp = static_cast<float>(channel->mScalingKeys[scaleIndex].mTime);
				animData.mScales[scaleIndex] = data;
			}

			engineAnim.mBones.push_back(Bone(channel->mNodeName.data, animData));
		}

		returnValue.emplace_back(AnimationImporter::Import(file, myVersion, engineAnim));

	}

	if (anyErrors)
	{
		return Importer::ImportResult{};
	}

	std::vector<DummyAsset<StaticMesh>> dummyStaticMeshes{};
	std::vector<DummyAsset<SkinnedMesh>> dummySkinnedMeshes{};
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

				if (scene->mMeshes[node.mMeshes[i]]->HasBones())
				{
					SkinnedMeshComponent& meshComponent = reg.AddComponent<SkinnedMeshComponent>(meshHolder);

					meshComponent.mSkinnedMesh = dummySkinnedMeshes.emplace_back(GetMeshName(file, 
						scene->mMeshes[node.mMeshes[i]]->mName.C_Str())).mHandle;
					meshComponent.mMaterial =  std::move(mat);
				}
				else
				{
					StaticMeshComponent& meshComponent = reg.AddComponent<StaticMeshComponent>(meshHolder);

					meshComponent.mStaticMesh = dummyStaticMeshes.emplace_back(GetMeshName(file,
						scene->mMeshes[node.mMeshes[i]]->mName.C_Str())).mHandle;
					meshComponent.mMaterial = std::move(mat);
				}

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

	std::optional<ImportedAsset> importedPrefab = PrefabImporter::MakePrefabFromEntity(file, GetSceneName(file), myVersion, world, prefabEntity);

	if (importedPrefab.has_value())
	{
		returnValue.emplace_back(std::move(*importedPrefab));
		return returnValue;
	}
	
	LOG(LogError, Error, "Importing {} failed; failed to make a valid prefab out of the ai scene", file.string());
	return Importer::ImportResult{};
}

std::optional<CE::ImportedAsset> CE::ModelImporter::ImportFromMemory(const std::filesystem::path& importedFromFile,
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

std::optional<CE::ImportedAsset> CE::ModelImporter::ImportFromMemory(const std::filesystem::path& importedFromFile,
    const std::string& name, 
    const uint32 importerVersion, 
    Span<const glm::vec3> positions, 
	std::optional<std::variant<Span<const uint16>, Span<const uint32>>> indices,
    std::optional<Span<const glm::vec3>> normals,
	std::optional<Span<const glm::vec3>> tangents,
    std::optional<Span<const glm::vec2>> textureCoordinates,
	std::optional<Span<const glm::ivec4>> boneIds,
	std::optional<Span<const glm::vec4>> boneWeights,
	std::optional<std::unordered_map<std::string, BoneInfo>> boneMap)
{
	const MetaType* const skinnedMeshType = MetaManager::Get().TryGetType<SkinnedMesh>();
	ASSERT(skinnedMeshType != nullptr);

    ImportedAsset importedMesh{ name, *skinnedMeshType, importedFromFile, importerVersion};
    if (SkinnedMesh::OnSave(importedMesh, positions, indices, normals, tangents, textureCoordinates, boneIds, boneWeights, boneMap))
    {
        return importedMesh;
    }
    return std::optional<ImportedAsset>();
}

CE::MetaType CE::ModelImporter::Reflect()
{
	MetaType type = MetaType{ MetaType::T<ModelImporter>{}, "ModelImporter", MetaType::Base<Importer>{} };

	SetClassVersion(type, 2);

	return type;
}
