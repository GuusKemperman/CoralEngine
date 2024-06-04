#include "Precomp.h"
#include "Platform/PC/Rendering/MeshPC.h"

#include <numeric>

#include "Assets/Core/AssetLoadInfo.h"
#include "Assets/Asset.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Utilities/Reflect/ReflectAssetType.h"
#include "Core/Device.h"
#include "Utilities/Math.h"

enum StaticMeshFlags : uint8
{
    hasIndices = 1,
    hasNormals = 1 << 1,
    hasUVs = 1 << 2,
    hasColors = 1 << 3, // No longer used
    areIndices16Bit = 1 << 4,
    hasTangents = 1 << 5
};

struct CE::StaticMesh::DXImpl
{
    std::shared_ptr<DXResource> mVertexBuffer{};
    std::shared_ptr<DXResource> mNormalBuffer{};
    std::shared_ptr<DXResource> mTangentBuffer{};
    std::shared_ptr<DXResource> mTexCoordBuffer{};
    std::shared_ptr<DXResource> mIndexBuffer{};

    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView{};
    D3D12_VERTEX_BUFFER_VIEW mNormalBufferView{};
    D3D12_VERTEX_BUFFER_VIEW mTexCoordBufferView{};
    D3D12_VERTEX_BUFFER_VIEW mTangentBufferView{};
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView{};

    int mIndexCount = 0;
    int mVertexCount = 0;
    DXGI_FORMAT mIndexFormat{};
    bool mBeenUpdated = false;
};

CE::StaticMesh::StaticMesh(std::string_view name) :
    Asset(name, MakeTypeId<StaticMesh>()),
    mImpl(new DXImpl)
{}

CE::StaticMesh::StaticMesh(AssetLoadInfo& loadInfo) :
    Asset(loadInfo),
    mImpl(new DXImpl())
{
    std::istream& str = loadInfo.GetStream();

    StaticMeshFlags flags{};
    str.read(reinterpret_cast<char*>(&flags), sizeof(StaticMeshFlags));

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
        std::optional<std::vector<glm::vec3>> optTangents = Math::CalculateTangents(indices.data(),
            numOfIndices,
            flags & areIndices16Bit,
            reinterpret_cast<glm::vec3*>(positions.data()),
            normals,
            UVs, 
            numOfVertices);


        if (optTangents.has_value())
        {
            tangentsStorage = std::move(*optTangents);
            tangents = tangentsStorage.data();
        }
    }

    bool meshLoaded = LoadMesh(indices.data(),
        numOfIndices,
        indicesSizeOfType,
        reinterpret_cast<const float*>(&positions.data()->x),
        reinterpret_cast<const float*>(&normals->x),
        reinterpret_cast<const float*>(&UVs->x),
        reinterpret_cast<const float*>(&tangents->x),
        numOfVertices
    );

    if (meshLoaded)
    {
#ifdef EDITOR
        // There is no reason why this NEEDS to be editor only,
        // feel free to remove all the ifdefs if you require this
        // data in non-editor builds. But since we likely won't need
        // it for non-editor builds and because it does take up additional
        // RAM, these buffers are, for now, editor only.
        mCPUVertexBuffer = std::move(positions);
        mCPUIndexBuffer = flags & areIndices16Bit ?
            std::vector<uint32>{ reinterpret_cast<const uint16*>(indices.data()), reinterpret_cast<const uint16*>(indices.data()) + numOfIndices } :
            std::vector<uint32>{ reinterpret_cast<const uint32*>(indices.data()), reinterpret_cast<const uint32*>(indices.data()) + numOfIndices };
        mBoundingBox = { GetVertices() };
#endif
    }
    else
    {
        LOG(LogAssets, Error, "Loading of {} failed: Invalid mesh", GetName());
    }
}

CE::StaticMesh::StaticMesh(StaticMesh&& other) noexcept = default;

void CE::StaticMesh::DrawMesh() const
{
    if (mImpl->mVertexBuffer == nullptr)
        return;

	Device& engineDevice = Device::Get();
	ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

	commandList->IASetVertexBuffers(0, 1, &mImpl->mVertexBufferView);
	commandList->IASetVertexBuffers(1, 1, &mImpl->mNormalBufferView);
    commandList->IASetVertexBuffers(2, 1, &mImpl->mTangentBufferView);
	commandList->IASetVertexBuffers(3, 1, &mImpl->mTexCoordBufferView);
	commandList->IASetIndexBuffer(&mImpl->mIndexBufferView);
	commandList->DrawIndexedInstanced(mImpl->mIndexCount, 1, 0, 0, 0);
}

void CE::StaticMesh::DrawMeshVertexOnly() const
{
    if (mImpl->mVertexBuffer == nullptr)
        return;

    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

    commandList->IASetVertexBuffers(0, 1, &mImpl->mVertexBufferView);
    commandList->IASetIndexBuffer(&mImpl->mIndexBufferView);
    commandList->DrawIndexedInstanced(mImpl->mIndexCount, 1, 0, 0, 0);
}

bool CE::StaticMesh::LoadMesh(const char* indices, unsigned int indexCount, unsigned int sizeOfIndexType, const float* positions, const float* normalsBuffer, const float* textureCoordinates, const float* tangents, unsigned int vertexCount)
{
	if (Device::IsHeadless() ||
		indices == nullptr ||
		indexCount == 0 ||
		positions == nullptr ||
		vertexCount == 0 ||
		sizeOfIndexType == 0) {
		return false;
	}

	switch (sizeOfIndexType)
	{
		case sizeof(unsigned char):			mImpl->mIndexFormat = DXGI_FORMAT_R8_UINT; break;
		case sizeof(unsigned short):		mImpl->mIndexFormat = DXGI_FORMAT_R16_UINT; break;
		case sizeof(unsigned int):			mImpl->mIndexFormat = DXGI_FORMAT_R32_UINT; break;
		default: return false;
	}


	Device& engineDevice = Device::Get();
	ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
	ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());
    engineDevice.StartUploadCommands();
	mImpl->mIndexCount = indexCount;
	mImpl->mVertexCount = vertexCount;

	int iBufferSize = sizeOfIndexType * mImpl->mIndexCount;
	int nBufferSize = sizeof(float) * mImpl->mVertexCount * 3;
	int tBufferSize = sizeof(float) * mImpl->mVertexCount * 2;
	int tanBufferSize = sizeof(float) * mImpl->mVertexCount * 3;

	mImpl->mVertexBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(nBufferSize), nullptr, "Vertex resource buffer");
	mImpl->mTexCoordBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(tBufferSize), nullptr, "Texture coord resource buffer");
	mImpl->mNormalBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(nBufferSize), nullptr, "Normals resource buffer");
	mImpl->mTangentBuffer = std::make_shared<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(tanBufferSize), nullptr, "Tangent resource buffer");

	D3D12_SUBRESOURCE_DATA vData = {};
	vData.pData = positions;
	vData.RowPitch = sizeof(float) * 3;
	vData.SlicePitch = nBufferSize;
	mImpl->mVertexBuffer->CreateUploadBuffer(device, nBufferSize, 0);
	mImpl->mVertexBuffer->Update(uploadCmdList, vData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);

	if (textureCoordinates) {
		D3D12_SUBRESOURCE_DATA tData = {};
		tData.pData = textureCoordinates;
		tData.RowPitch = sizeof(float) * 2;
		tData.SlicePitch = tBufferSize;
		mImpl->mTexCoordBuffer->CreateUploadBuffer(device, tBufferSize, 0);
		mImpl->mTexCoordBuffer->Update(uploadCmdList, tData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
	}

	if (normalsBuffer) {
		D3D12_SUBRESOURCE_DATA nData = {};
		nData.pData = normalsBuffer;
		nData.RowPitch = sizeof(float) * 3;
		nData.SlicePitch = nBufferSize;
		mImpl->mNormalBuffer->CreateUploadBuffer(device, nBufferSize, 0);
		mImpl->mNormalBuffer->Update(uploadCmdList, nData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
	}

	if (tangents) {
		D3D12_SUBRESOURCE_DATA tanData = {};
		tanData.pData = tangents;
		tanData.RowPitch = sizeof(float) * 3;
		tanData.SlicePitch = tanBufferSize;
		mImpl->mTangentBuffer->CreateUploadBuffer(device, tanBufferSize, 0);
		mImpl->mTangentBuffer->Update(uploadCmdList, tanData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
	}

    mImpl->mIndexBuffer = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), nullptr, "Index resource buffer");
	D3D12_SUBRESOURCE_DATA iData = {};
	iData.pData = indices;
	iData.RowPitch = iBufferSize;
	iData.SlicePitch = iBufferSize;
    mImpl->mIndexBuffer->CreateUploadBuffer(device, iBufferSize, 0);
    mImpl->mIndexBuffer->Update(uploadCmdList, iData, D3D12_RESOURCE_STATE_INDEX_BUFFER, 0, 1);

	mImpl->mVertexBufferView.BufferLocation = mImpl->mVertexBuffer->GetResource()->GetGPUVirtualAddress();
	mImpl->mVertexBufferView.StrideInBytes = sizeof(float) * 3;
	mImpl->mVertexBufferView.SizeInBytes = nBufferSize;

	mImpl->mNormalBufferView.BufferLocation = mImpl->mNormalBuffer->GetResource()->GetGPUVirtualAddress();
	mImpl->mNormalBufferView.StrideInBytes = sizeof(float) * 3;
	mImpl->mNormalBufferView.SizeInBytes = nBufferSize;

	mImpl->mTexCoordBufferView.BufferLocation = mImpl->mTexCoordBuffer->GetResource()->GetGPUVirtualAddress();
	mImpl->mTexCoordBufferView.StrideInBytes = sizeof(float) * 2;
	mImpl->mTexCoordBufferView.SizeInBytes = tBufferSize;

	mImpl->mTangentBufferView.BufferLocation = mImpl->mTangentBuffer->GetResource()->GetGPUVirtualAddress();
	mImpl->mTangentBufferView.StrideInBytes = sizeof(float) * 3;
	mImpl->mTangentBufferView.SizeInBytes = tanBufferSize;

	mImpl->mIndexBufferView.BufferLocation = mImpl->mIndexBuffer->GetResource()->GetGPUVirtualAddress();
	mImpl->mIndexBufferView.Format = mImpl->mIndexFormat;
	mImpl->mIndexBufferView.SizeInBytes = iBufferSize;
    engineDevice.SubmitUploadCommands();
	return true;
}

void CE::StaticMesh::DXImplDeleter::operator()(DXImpl* impl) const
{
    delete impl;
}
