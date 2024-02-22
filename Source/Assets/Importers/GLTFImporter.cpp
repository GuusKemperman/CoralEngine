#include "Precomp.h"
#include "Assets/Importers/GLTFImporter.h"

#undef APIENTRY

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4018)
#pragma warning(disable : 4267)
#endif

#include "tinygltf/tiny_gltf.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "World/World.h"
#include "World/Registry.h"
#include "Assets/StaticMesh.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Components/TransformComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Assets/Importers/TextureImporter.h"
#include "Assets/Importers/MaterialImporter.h"
#include "Assets/Importers/StaticMeshImporter.h"
#include "Assets/Importers/PrefabImporter.h"
#include "Components/NameComponent.h"
#include "Utilities/ClassVersion.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaType.h"

static std::string GetTexName(const std::filesystem::path& modelPath, const tinygltf::Texture& texture)
{
	return texture.name.empty() ? modelPath.filename().replace_extension().string().append("_tex").append(std::to_string(texture.source)) :
		modelPath.filename().replace_extension().string().append("_").append(texture.name);
}

static std::string GetMeshName(const std::filesystem::path& modelPath, const tinygltf::Mesh& mesh, const size_t)
{
	return modelPath.filename().replace_extension().string().append("_").append(mesh.name);
}

static std::string Get***REMOVED***neName(const std::filesystem::path& modelPath, const tinygltf::***REMOVED***ne& ***REMOVED***ne)
{
	return modelPath.filename().replace_extension().string().append("_").append(***REMOVED***ne.name);
}

std::optional<std::vector<Engine::ImportedAsset>> Engine::GLTFImporter::Import(const std::filesystem::path& path) const
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool success{};

	if (path.extension() == ".glb")
	{
		success = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
	}
	else if (path.extension() == ".gltf")
	{
		success = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
	}
	else
	{
		LOG(LogAssets, Error, "Invalid extension: {}", path.extension().string());
		return std::nullopt;
	}

	if (!err.empty()
		|| !success)
	{
		LOG(LogAssets, Error, "Error emitted while loading {}: {}", path.string(), err);
		return std::nullopt;
	}

	if (!warn.empty())
	{
		LOG(LogAssets, Warning, "Warning emitted while loading {}: {}", path.string(), warn);
	}

	const MetaType* const myType = MetaManager::Get().TryGetType<GLTFImporter>();
	ASSERT(myType != nullptr);
	const uint32 myVersion = GetClassVersion(*myType);

	std::vector<ImportedAsset> returnValue{};

	// Not really loaded in, but materials hold a shared ptr to textures
	std::vector<std::shared_ptr<const Texture>> textures{};

	// We set it to false initially. If we encounter errors, we continue importing for a bit (but set this to false), 
	// just so we can log more errors and inform the user of any other issues.
	bool anyErrors = false;

	for (const tinygltf::Texture& gltfTexture : model.textures)
	{
		std::string textureName = GetTexName(path, gltfTexture);
		const tinygltf::Image& image = model.images[gltfTexture.source];
		
		static_assert(std::is_same_v<decltype(image.image[0]), const unsigned char&>, "The reintpret_cast a few lines down is invalid");

		std::optional<ImportedAsset> importedTexture = TextureImporter::ImportFromMemory(path,
			textureName,
			myVersion,
			reinterpret_cast<const std::vector<char>&>(image.image), 
			static_cast<uint32>(image.width),
			static_cast<uint32>(image.height));

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

	std::vector<std::shared_ptr<const Material>> materials{};

	for (const tinygltf::Material& gltfMat : model.materials)
	{
		Material& engineMat = const_cast<Material&>(*materials.emplace_back(std::make_shared<const Material>(gltfMat.name)));

		engineMat.mBaseColorFactor = {
			static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[0]),
			static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[1]),
			static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[2]),
			static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[3])
		};
		engineMat.mMetallicFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.metallicFactor);
		engineMat.mRoughnessFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.roughnessFactor);
		engineMat.mEmissiveFactor = {
			static_cast<float>(gltfMat.emissiveFactor[0]),
			static_cast<float>(gltfMat.emissiveFactor[1]),
			static_cast<float>(gltfMat.emissiveFactor[2])
		};
		engineMat.mAlphaCutoff = static_cast<float>(gltfMat.alphaCutoff);
		engineMat.mDoubleSided = gltfMat.doubleSided;
		engineMat.mNormalScale = static_cast<float>(gltfMat.normalTexture.scale);
		engineMat.mOcclusionStrength = static_cast<float>(gltfMat.occlusionTexture.strength);

		engineMat.mBaseColorTexture = gltfMat.pbrMetallicRoughness.baseColorTexture.index >= 0 ? textures[gltfMat.pbrMetallicRoughness.baseColorTexture.index] : nullptr;
		engineMat.mMetallicRoughnessTexture = gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 ? textures[gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index] : nullptr;
		engineMat.mNormalTexture = gltfMat.normalTexture.index >= 0 ? textures[gltfMat.normalTexture.index] : nullptr;
		engineMat.mOcclusionTexture = gltfMat.occlusionTexture.index >= 0 ?	textures[gltfMat.occlusionTexture.index] : nullptr;
		engineMat.mEmissiveTexture = gltfMat.emissiveTexture.index >= 0 ? textures[gltfMat.emissiveTexture.index] : nullptr;

		returnValue.emplace_back(MaterialImporter::Import(
			path,
			myVersion,
			engineMat));
	}

	struct GLTFMeshPrimitives
	{
		// A gltf mesh contains many primitives.
		// in our engine, a primitive becomes it's own static mesh.
		struct MeshMat
		{
			MeshMat(std::shared_ptr<const StaticMesh>&& mesh, std::shared_ptr<const Material>&& material) :
				mStaticMesh(std::move(mesh)),
				mMaterial(std::move(material)){}
			std::shared_ptr<const StaticMesh> mStaticMesh{};
			std::shared_ptr<const Material> mMaterial{};
		};
		std::vector<MeshMat> mMeshMats{};
	};
	std::vector<GLTFMeshPrimitives> staticMeshMakers{};

	staticMeshMakers.resize(model.meshes.size());

	for (size_t i = 0; i < model.meshes.size(); i++)
	{
		const tinygltf::Mesh& gltfMesh = model.meshes[i];
		GLTFMeshPrimitives& meshMaker = staticMeshMakers[i];

		for (size_t j = 0; j < gltfMesh.primitives.size(); j++)
		{
			const tinygltf::Primitive& primitive = gltfMesh.primitives[j];
			const std::string meshName = GetMeshName(path, gltfMesh, j);

			std::variant<Span<const uint16>, Span<const uint32>> indices{};
			
			const tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
			const tinygltf::Buffer& indicesBuffer = model.buffers[indicesBufferView.mBuffers];
			const unsigned char* indicesData = &indicesBuffer.data[indicesBufferView.byteOffset];

			switch (indicesAccessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			{
				const unsigned* start = reinterpret_cast<const unsigned*>(indicesData);
				indices = Span<const uint32>{ start, start + indicesAccessor.count };
				break;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			{
				const unsigned short* start = reinterpret_cast<const unsigned short*>(indicesData);
				indices = Span<const uint16>{ start, start + indicesAccessor.count };
				break;
			}
			default:
				LOG(LogAssets, Error, "Importing mesh {} failed: Indices type not handles. Use unsigned int or unsigned shorts.", meshName);
				anyErrors = true;
				break;
			}

			Span<const glm::vec3> positions{};
			std::optional<Span<const glm::vec3>> normals{};
			std::optional<Span<const glm::vec2>> textureCoordinates{};
			std::optional<Span<const glm::vec3>> colors{};

			for (const auto& [attribName, accessorIndex] : primitive.attributes)
			{
				const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& mBuffers = model.buffers[bufferView.mBuffers];

				const unsigned char* data = &mBuffers.data[bufferView.byteOffset];

				if (attribName.compare("POSITION") == 0)
				{
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						positions = { reinterpret_cast<const glm::vec3*>(data), accessor.count };
					}
					else
					{
						LOG(LogAssets, Error, "Importing mesh {} failed: position type not handled. Use floats.", meshName);
						anyErrors = true;
					}
				}
				else if (attribName.compare("NORMAL") == 0)
				{
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						normals = { reinterpret_cast<const glm::vec3*>(data), accessor.count };
					}
					else
					{
						LOG(LogAssets, Warning, "While importing mesh {}: normal type not handled. Use floats.", meshName);
					}
				}
				else if (attribName.compare("TEXCOORD_0") == 0)
				{
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						textureCoordinates = { reinterpret_cast<const glm::vec2*>(data), accessor.count };
					}
					else
					{
						LOG(LogAssets, Warning, "While importing mesh {}: texcoord type not handled. Use floats.", meshName);
					}
				}
				else if (attribName.compare("COLOR_0") == 0)
				{
					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						colors = { reinterpret_cast<const glm::vec3*>(data), accessor.count };
					}
					else
					{
						LOG(LogAssets, Warning, "While importing mesh {}: color type not handled. Use floats.", meshName);
					}
				}
				else
				{
					LOG(LogAssets, Verbose, "Ignored attribute {}", attribName);
				}
			}

			if (anyErrors)
			{
				continue;
			}

			std::optional<ImportedAsset> importedAsset = StaticMeshImporter::ImportFromMemory(path,
				meshName,
				myVersion,
				positions,
				indices,
				normals,
				textureCoordinates,
				colors);

			if (importedAsset.has_value())
			{
				returnValue.push_back(std::move(*importedAsset));
			}
			else
			{
				LOG(LogAssets, Error, "Failed to import mesh {}", meshName);
				anyErrors = true;
			}

			meshMaker.mMeshMats.emplace_back(std::make_shared<const StaticMesh>(meshName), primitive.material == -1 ? Material::TryGetDefaultMaterial() : materials[primitive.material]);
		}
	}

	if (anyErrors)
	{
		return std::nullopt;
	}

	World world{ false };
	Registry& registry = world.GetRegistry();

	for (const tinygltf::***REMOVED***ne& ***REMOVED***ne : model.***REMOVED***nes)
	{
		const std::string ***REMOVED***neName = Get***REMOVED***neName(path, ***REMOVED***ne);

		const entt::entity prefabMainEntity = registry.Create();
		registry.AddComponent<NameComponent>(prefabMainEntity, ***REMOVED***neName);

		auto& prefabTransform = registry.AddComponent<TransformComponent>(prefabMainEntity);

		std::vector<entt::entity> entities{};
		entities.resize(model.nodes.size());
		registry.Create(entities.begin(), entities.end());
		registry.AddComponents<NameComponent>(entities.begin(), entities.end());

		for (const auto entity : entities)
		{
			registry.AddComponent<TransformComponent>(entity).SetParent(&prefabTransform);
		}

		for (size_t i = 0; i < model.nodes.size(); i++)
		{
			const tinygltf::Node& node = model.nodes[i];

			entt::entity current = entities[i];
			registry.Get<NameComponent>(current).mName = node.name;

			auto& currentTransform = registry.Get<TransformComponent>(current);

			for (const int childIndex : node.children)
			{
				entt::entity childEntity = entities[childIndex];
				auto& childTransform = registry.Get<TransformComponent>(childEntity);
				childTransform.SetParent(&currentTransform);
			}

			if (!node.matrix.empty())
			{
				currentTransform.SetLocalMatrix(static_cast<glm::mat4>(*reinterpret_cast<const glm::mat<4, 4, double>*>(node.matrix.data())));
			}
			else
			{
				if (!node.translation.empty())
				{
					currentTransform.SetLocalPosition(static_cast<glm::vec3>(*reinterpret_cast<const glm::vec<3, double>*>(node.translation.data())));
				}

				if (!node.rotation.empty())
				{
					const glm::quat rotation =
					{
						static_cast<float>(node.rotation[3]),
						static_cast<float>(node.rotation[0]),
						static_cast<float>(node.rotation[1]),
						static_cast<float>(node.rotation[2])
					};
					currentTransform.SetLocalOrientation(rotation);
				}

				if (!node.scale.empty())
				{
					currentTransform.SetLocalScale(static_cast<glm::vec3>(*reinterpret_cast<const glm::vec<3, double>*>(node.scale.data())));
				}
			}

			if (node.mesh >= 0)
			{
				const auto& engineMesh = staticMeshMakers[node.mesh];

				for (size_t j = 0; j < engineMesh.mMeshMats.size(); j++)
				{
					const auto& meshAndMat = engineMesh.mMeshMats[j];
					entt::entity entityToAddMeshTo{};

					if (j == 0)
					{
						entityToAddMeshTo = current;
					}
					else
					{
						entityToAddMeshTo = registry.Create();
						registry.AddComponent<NameComponent>(entityToAddMeshTo, meshAndMat.mStaticMesh->GetName());
						registry.AddComponent<TransformComponent>(entityToAddMeshTo).SetParent(&currentTransform);
					}

					auto& staticMeshComponent = registry.AddComponent<StaticMeshComponent>(entityToAddMeshTo);
					staticMeshComponent.mStaticMesh = meshAndMat.mStaticMesh;
					staticMeshComponent.mMaterial = meshAndMat.mMaterial;
				}
			}
		}

		returnValue.emplace_back(PrefabImporter::MakePrefabFromEntity(path, ***REMOVED***neName, myVersion, world, prefabMainEntity));
	}

	return returnValue;
}

Engine::MetaType Engine::GLTFImporter::Reflect()
{
	MetaType type = MetaType{ MetaType::T<GLTFImporter>{}, "GLTFImporter", MetaType::Base<Importer>{} };
	return type;
}
