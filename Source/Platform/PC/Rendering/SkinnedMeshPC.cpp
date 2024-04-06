#include "Precomp.h"
#include "Platform/PC/Rendering/SkinnedMeshPC.h"
#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Asset.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Core/Device.h"
#include "Meta/MetaManager.h"
#include "Utilities/Math.h"
#include <numeric> 

namespace cereal
{
    inline void save(BinaryOutputArchive& ar, const CE::BoneInfo& value)
    {
        ar(value.mOffset, value.mId);
    }

    inline void load(BinaryInputArchive& ar, CE::BoneInfo& value)
    {
        ar(value.mOffset, value.mId);
    }
}

enum SkinnedMeshFlags : uint8
{
    hasIndices = 1,
    hasNormals = 1 << 1,
    hasUVs = 1 << 2,
    hasColors = 1 << 3, // No longer used
    areIndices16Bit = 1 << 4,
    hasTangents = 1 << 5,
    hasBoneIds = 1 << 6,
    hasBoneWeights = 1 << 7
};

CE::SkinnedMesh::SkinnedMesh(AssetLoadInfo& loadInfo) :
    Asset(loadInfo)
{
    std::istream& str = loadInfo.GetStream();

    SkinnedMeshFlags flags{};
    str.read(reinterpret_cast<char*>(&flags), sizeof(flags));

    uint32 numOfVertices{};
    str.read(reinterpret_cast<char*>(&numOfVertices), sizeof(numOfVertices));

    std::vector<glm::vec3> positions(numOfVertices);
    str.read(reinterpret_cast<char*>(positions.data()), numOfVertices * sizeof(glm::vec3));

    std::vector<char> indices{};
    const uint32 indicesSizeOfType = flags & areIndices16Bit ? sizeof(uint16) : sizeof(uint32);

    uint32 numOfIndices{};

    if (flags & hasIndices)
    {
        str.read(reinterpret_cast<char*>(&numOfIndices), sizeof(numOfIndices));

        indices.resize(numOfIndices * indicesSizeOfType);
        str.read(reinterpret_cast<char*>(indices.data()), numOfIndices * indicesSizeOfType);
    }
    else
    {
        numOfIndices = static_cast<uint32>(positions.size());
        indices.resize(positions.size());
        std::iota(indices.begin(), indices.end(), 1);
    }

    std::vector<glm::vec3> normalsStorage(0);
    const glm::vec3* normals = nullptr;

    if (flags & hasNormals)
    {
        normalsStorage.resize(numOfVertices);
        str.read(reinterpret_cast<char*>(normalsStorage.data()), numOfVertices * sizeof(glm::vec3));
        normals = normalsStorage.data();
    }

    std::vector<glm::vec2> UVsStorage(0);
    const glm::vec2* UVs = nullptr;

    if (flags & hasUVs)
    {
        UVsStorage.resize(numOfVertices);
        str.read(reinterpret_cast<char*>(UVsStorage.data()), numOfVertices * sizeof(glm::vec2));
        UVs = UVsStorage.data();
    }

    std::vector<glm::vec3> tangentsStorage(0);
    const glm::vec3* tangents = nullptr;

    int loadInfoVersion = loadInfo.GetMetaData().GetAssetVersion();
    if (loadInfoVersion == 1
        && flags & hasTangents)
    {
        tangentsStorage.resize(numOfVertices);
        str.read(reinterpret_cast<char*>(tangentsStorage.data()), numOfVertices * sizeof(glm::vec3));
        tangents = tangentsStorage.data();
    }
    else
    {
        std::optional<std::vector<glm::vec3>> optTangents = Math::CalculateTangents(indices.data(), numOfIndices, flags & areIndices16Bit, positions.data(), normals, UVs, numOfVertices);

        if (optTangents.has_value())
        {
            tangentsStorage = std::move(*optTangents);
            tangents = tangentsStorage.data();
        }
    }

    std::vector<glm::ivec4> boneIdStorage(0);
    const glm::ivec4* boneIds = nullptr;

    if (flags & hasBoneIds)
    {
        boneIdStorage.resize(numOfVertices);
        str.read(reinterpret_cast<char*>(boneIdStorage.data()), numOfVertices * sizeof(glm::ivec4));
        boneIds = boneIdStorage.data();
    }

    std::vector<glm::vec4> boneWeightStorage(0);
    const glm::vec4* boneWeights = nullptr;

    if (flags & hasBoneWeights)
    {
        boneWeightStorage.resize(numOfVertices);
        str.read(reinterpret_cast<char*>(boneWeightStorage.data()), numOfVertices * sizeof(glm::vec4));
        boneWeights = boneWeightStorage.data();
    }

    bool meshLoaded = LoadMesh(indices.data(),
        numOfIndices,
        indicesSizeOfType,
        reinterpret_cast<const float*>(&positions.data()->x),
        reinterpret_cast<const float*>(&normals->x),
        reinterpret_cast<const float*>(&UVs->x),
        reinterpret_cast<const float*>(&tangents->x),
        reinterpret_cast<const int*>(&boneIds->x),
        reinterpret_cast<const float*>(&boneWeights->x),
        numOfVertices
    );

    BinaryGSONObject obj {};

    const bool success = obj.LoadFromBinary(loadInfo.GetStream());
    
    if (!success)
    {
        LOG(LogAssets, Error, "Could not load skinned mesh {}, GSON parsing failed", GetName());
        return;
    }

    const BinaryGSONMember* serializedBoneMap = obj.TryGetGSONMember("BoneMap");

    if (serializedBoneMap == nullptr)
    {
        LOG(LogAssets, Error, "Could not load skinned mesh {}, bone map is missing", GetName());
        return;
    }

    *serializedBoneMap >> mBoneInfoMap;

    if (!meshLoaded)
    {
        LOG(LogAssets, Error, "Loading of {} failed: Invalid mesh", GetName());
    }
}

CE::SkinnedMesh::SkinnedMesh(SkinnedMesh&& other) noexcept = default;

void CE::SkinnedMesh::DrawMesh() const
{
    if (mVertexBuffer == nullptr)
        return;

	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

	commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	commandList->IASetVertexBuffers(1, 1, &mNormalBufferView);
	commandList->IASetVertexBuffers(2, 1, &mTexCoordBufferView);
	commandList->IASetVertexBuffers(3, 1, &mTangentBufferView);
    commandList->IASetVertexBuffers(4, 1, &mBoneIdBufferView);
    commandList->IASetVertexBuffers(5, 1, &mBoneWeightBufferView);
	commandList->IASetIndexBuffer(&mIndexBufferView);
	commandList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
}

void CE::SkinnedMesh::DrawMeshVertexOnly() const
{
    if (mVertexBuffer == nullptr)
        return;

    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

    commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    commandList->IASetVertexBuffers(1, 1, &mBoneIdBufferView);
    commandList->IASetVertexBuffers(2, 1, &mBoneWeightBufferView);
    commandList->IASetIndexBuffer(&mIndexBufferView);
    commandList->DrawIndexedInstanced(mIndexCount, 1, 0, 0, 0);
}

bool CE::SkinnedMesh::LoadMesh(const char* indices, unsigned int indexCount, unsigned int sizeOfIndexType, const float* positions, const float* normalsBuffer, const float* textureCoordinates, const float* tangents, const int* boneIds, const float* boneWeights, unsigned int vertexCount)
{
	if (indices == nullptr ||
		indexCount == 0 ||
		positions == nullptr ||
		vertexCount == 0 ||
		sizeOfIndexType == 0) {
		return false;
	}

	switch (sizeOfIndexType)
	{
		case sizeof(unsigned char):			mIndexFormat = DXGI_FORMAT_R8_UINT; break;
			case sizeof(unsigned short):		mIndexFormat = DXGI_FORMAT_R16_UINT; break;
				case sizeof(unsigned int):			mIndexFormat = DXGI_FORMAT_R32_UINT; break;
				default: return false;
	}


	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());
    engineDevice.StartUploadCommands();
	mIndexCount = indexCount;
	mVertexCount = vertexCount;

	int iBufferSize = sizeOfIndexType * mIndexCount;
	int nBufferSize = sizeof(float) * mVertexCount * 3;
	int tBufferSize = sizeof(float) * mVertexCount * 2;
	int tanBufferSize = sizeof(float) * mVertexCount * 3;
    int boneIdBufferSize = sizeof(int) * mVertexCount * 4;
    int boneWeightBufferSize = sizeof(float) * mVertexCount * 4;

	mVertexBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(nBufferSize), nullptr, "Vertex resource buffer");
	mTexCoordBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(tBufferSize), nullptr, "Texture coord resource buffer");
	mNormalBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(nBufferSize), nullptr, "Normals resource buffer");
	mTangentBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(tanBufferSize), nullptr, "Tangent resource buffer");
    mBoneIdBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(boneIdBufferSize), nullptr, "BoneId resource buffer");
    mBoneWeightBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(boneWeightBufferSize), nullptr, "BoneWeight resource buffer");

	D3D12_SUBRESOURCE_DATA vData = {};
	vData.pData = positions;
	vData.RowPitch = sizeof(float) * 3;
	vData.SlicePitch = nBufferSize;
	mVertexBuffer->CreateUploadBuffer(device, nBufferSize, 0);
	mVertexBuffer->Update(uploadCmdList, vData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);

	if (textureCoordinates) {
		D3D12_SUBRESOURCE_DATA tData = {};
		tData.pData = textureCoordinates;
		tData.RowPitch = sizeof(float) * 2;
		tData.SlicePitch = tBufferSize;
		mTexCoordBuffer->CreateUploadBuffer(device, tBufferSize, 0);
		mTexCoordBuffer->Update(uploadCmdList, tData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
	}

	if (normalsBuffer) {
		D3D12_SUBRESOURCE_DATA nData = {};
		nData.pData = normalsBuffer;
		nData.RowPitch = sizeof(float) * 3;
		nData.SlicePitch = nBufferSize;
		mNormalBuffer->CreateUploadBuffer(device, nBufferSize, 0);
		mNormalBuffer->Update(uploadCmdList, nData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
	}

	if (tangents) {
		D3D12_SUBRESOURCE_DATA tanData = {};
		tanData.pData = tangents;
		tanData.RowPitch = sizeof(float) * 3;
		tanData.SlicePitch = tanBufferSize;
		mTangentBuffer->CreateUploadBuffer(device, tanBufferSize, 0);
		mTangentBuffer->Update(uploadCmdList, tanData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
	}

    if (boneIds) {
        D3D12_SUBRESOURCE_DATA boneIdData = {};
        boneIdData.pData = boneIds;
        boneIdData.RowPitch = sizeof(int) * 4;
        boneIdData.SlicePitch = boneIdBufferSize;
        mBoneIdBuffer->CreateUploadBuffer(device, boneIdBufferSize, 0);
        mBoneIdBuffer->Update(uploadCmdList, boneIdData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    }

    if (boneWeights)
    {
        D3D12_SUBRESOURCE_DATA boneWeightData = {};
        boneWeightData.pData = boneWeights;
        boneWeightData.RowPitch = sizeof(float) * 4;
        boneWeightData.SlicePitch = boneWeightBufferSize;
        mBoneWeightBuffer->CreateUploadBuffer(device, boneWeightBufferSize, 0);
        mBoneWeightBuffer->Update(uploadCmdList, boneWeightData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    }

	mIndexBuffer = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), nullptr, "Index resource buffer");
	D3D12_SUBRESOURCE_DATA iData = {};
	iData.pData = indices;
	iData.RowPitch = iBufferSize;
	iData.SlicePitch = iBufferSize;
	mIndexBuffer->CreateUploadBuffer(device, iBufferSize, 0);
	mIndexBuffer->Update(uploadCmdList, iData, D3D12_RESOURCE_STATE_INDEX_BUFFER, 0, 1);

	mVertexBufferView.BufferLocation = mVertexBuffer->GetResource()->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = sizeof(float) * 3;
	mVertexBufferView.SizeInBytes = nBufferSize;

	mNormalBufferView.BufferLocation = mNormalBuffer->GetResource()->GetGPUVirtualAddress();
	mNormalBufferView.StrideInBytes = sizeof(float) * 3;
	mNormalBufferView.SizeInBytes = nBufferSize;

	mTexCoordBufferView.BufferLocation = mTexCoordBuffer->GetResource()->GetGPUVirtualAddress();
	mTexCoordBufferView.StrideInBytes = sizeof(float) * 2;
	mTexCoordBufferView.SizeInBytes = tBufferSize;

	mTangentBufferView.BufferLocation = mTangentBuffer->GetResource()->GetGPUVirtualAddress();
	mTangentBufferView.StrideInBytes = sizeof(float) * 3;
	mTangentBufferView.SizeInBytes = tanBufferSize;

    mBoneIdBufferView.BufferLocation = mBoneIdBuffer->GetResource()->GetGPUVirtualAddress();
    mBoneIdBufferView.StrideInBytes = sizeof(int) * 4;
    mBoneIdBufferView.SizeInBytes = boneIdBufferSize;

    mBoneWeightBufferView.BufferLocation = mBoneWeightBuffer->GetResource()->GetGPUVirtualAddress();
    mBoneWeightBufferView.StrideInBytes = sizeof(float) * 4;
    mBoneWeightBufferView.SizeInBytes = boneWeightBufferSize;

	mIndexBufferView.BufferLocation = mIndexBuffer->GetResource()->GetGPUVirtualAddress();
	mIndexBufferView.Format = mIndexFormat;
	mIndexBufferView.SizeInBytes = iBufferSize;
    engineDevice.SubmitUploadCommands();
	return true;
}