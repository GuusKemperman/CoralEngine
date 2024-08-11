#include "Precomp.h"
#include "Assets/StaticMesh.h"

#include <numeric>

#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Core/AssetSaveInfo.h"
#include "Core/Renderer.h"
#include "Utilities/ClassVersion.h"
#include "Utilities/StringFunctions.h"
#include "Utilities/Reflect/ReflectAssetType.h"

namespace
{
	enum StaticMeshFlags : uint8
	{
		hasIndices = 1,
		hasNormals = 1 << 1,
		hasTextureCoords = 1 << 2,
		hasTangents = 1 << 3
	};
}

CE::StaticMesh::StaticMesh(std::string_view name) :
	Asset(name, MakeTypeId<StaticMesh>())
{}

CE::StaticMesh::StaticMesh(AssetLoadInfo& loadInfo) :
	Asset(loadInfo)
{
	const std::string contents = StringFunctions::StreamToString(loadInfo.GetStream());
	const char* contentsPtr = contents.data();

	const auto& readContents = [&contents, &contentsPtr]<typename T>(int amount = 1) -> const T&
	{
		const int numBytesToRead = sizeof(T) * amount;

		if (contentsPtr + numBytesToRead > contents.data() + contents.size())
		{
			throw std::underflow_error{ "Ran out of bytes" };
		}
		const T& returnValue = reinterpret_cast<const T&>(*contentsPtr);
		contentsPtr += numBytesToRead;
		return returnValue;
	};

	try
	{
		const StaticMeshFlags flags = readContents.operator()<StaticMeshFlags>();
		const uint32 numOfVertices = readContents.operator()<uint32>();
		const std::span positions = { &readContents.operator()<glm::vec3>(numOfVertices), numOfVertices };
		const uint32 numOfIndices = readContents.operator()<uint32>();
		const std::span indices = flags & hasIndices ? std::span{ &readContents.operator()<uint32>(numOfIndices), numOfIndices } : std::span<uint32>{};
		const std::span normals = flags & hasNormals ? std::span{ &readContents.operator()<glm::vec3>(numOfVertices), numOfVertices } : std::span<glm::vec3>{};
		const std::span textureCoordinates = flags & hasTextureCoords ? std::span{ &readContents.operator()<glm::vec2>(numOfVertices), numOfVertices } : std::span<glm::vec2>{};
		const std::span tangents = flags & hasTangents ? std::span{ &readContents.operator()<glm::vec3>(numOfVertices), numOfVertices } : std::span<glm::vec3>{};

		mImpl = Renderer::Get().CreateStaticMeshPlatformImpl(indices, positions, normals, tangents, textureCoordinates);

#ifdef EDITOR
		// There is no reason why this NEEDS to be editor only,
		// feel free to remove all the ifdefs if you require this
		// data in non-editor builds. But since we likely won't need
		// it for non-editor builds and because it does take up additional
		// RAM, these buffers are, for now, editor only.
		if (mImpl != nullptr)
		{
			mCPUVertexBuffer = { positions.begin(), positions.end() };
			mCPUIndexBuffer = { indices.begin(), indices.end() };
			mBoundingBox = { GetVertices() };
		}
#endif
	}
	catch (const std::underflow_error& err)
	{
		LOG(LogAssets, Error, "StaticMesh contained less bytes than expected - {}", err.what());
		return;
	}
}

CE::MetaType CE::StaticMesh::Reflect()
{
	MetaType type = MetaType{ MetaType::T<StaticMesh>{}, "StaticMesh", MetaType::Base<Asset>{}, MetaType::Ctor<AssetLoadInfo&>{}, MetaType::Ctor<std::string_view>{} };
	type.GetProperties().Add(Props::sCannotReferenceOtherAssetsTag);

	SetClassVersion(type, 1);

	ReflectAssetType<StaticMesh>(type);
	return type;
}

bool CE::Internal::SaveStaticMesh(AssetSaveInfo& saveInfo,
	std::span<const glm::vec3> positions,
	std::span<const uint32> indices,
	std::span<const glm::vec3> normals,
	std::span<const glm::vec3> tangents,
	std::span<const glm::vec2> textureCoordinates)
{
	if (indices.size() % 3 != 0)
	{
		LOG(LogAssets, Error, "Importing static mesh failed: {} indices provided, but this is not divisible by 3", indices.size());
		return false;
	}

	if (!normals.empty()
		&& positions.size() != normals.size())
	{
		LOG(LogAssets, Error, "Importing static mesh failed: expected {} normals, but received {}", positions.size(), normals.size());
		return false;
	}

	if (!textureCoordinates.empty()
		&& positions.size() != textureCoordinates.size())
	{
		LOG(LogAssets, Error, "Importing static mesh failed: expected {} textureCoordinates, but received {}", positions.size(), textureCoordinates.size());
		return false;
	}

	if (!tangents.empty()
		&& positions.size() != tangents.size())
	{
		LOG(LogAssets, Error, "Importing static mesh failed: expected {} tangents, but received {}", positions.size(), tangents.size());
		return false;
	}

	StaticMeshFlags flags{};

	const auto setFlag = [&flags](const auto& container, StaticMeshFlags bit)
		{
			if (!container.empty())
			{
				flags = static_cast<StaticMeshFlags>(flags | bit);
			}
		};

	setFlag(indices, hasIndices);
	setFlag(normals, hasNormals);
	setFlag(textureCoordinates, hasTextureCoords);
	setFlag(tangents, hasTangents);

	std::ostream& str = saveInfo.GetStream();
	str.write(reinterpret_cast<const char*>(&flags), sizeof(StaticMeshFlags));

	const uint32 numOfPositions = static_cast<uint32>(positions.size());
	str.write(reinterpret_cast<const char*>(&numOfPositions), sizeof(numOfPositions));

	const auto writeBuffer = [flags, &str](const auto& container, StaticMeshFlags bit)
		{
			if (flags & bit)
			{
				str.write(reinterpret_cast<const char*>(container.data()), container.size_bytes());
			}
		};

	writeBuffer(positions, flags);

	if (flags & hasIndices)
	{
		const uint32 numOfIndices = static_cast<uint32>(indices.size());
		str.write(reinterpret_cast<const char*>(&numOfIndices), sizeof(numOfIndices));
		writeBuffer(indices, hasIndices);
	}

	writeBuffer(normals, hasNormals);
	writeBuffer(textureCoordinates, hasTextureCoords);
	writeBuffer(tangents, hasTangents);

	return true;
}
