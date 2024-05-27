#include "Precomp.h"
#include "Platform/PC/Rendering/GPUWorldPC.h"

#include "World/World.h"
#include "World/WorldViewport.h"
#include "World/Registry.h"

#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Assets/SkinnedMesh.h"

#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/FogComponent.h"
#include "Components/OutlineComponent.h"
#include "Rendering/GPUWorld.h"

#include "Components/Particles/ParticleEmitterComponent.h"
#include "Components/Particles/ParticleColorComponent.h"
#include "Components/Particles/ParticleColorOverTimeComponent.h"
#include "Components/Particles/ParticleLightComponent.h"

#include "Platform/PC/Core/DevicePC.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Rendering/FrameBuffer.h"

CE::DebugRenderingData::DebugRenderingData()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    uint32 bufferSize = sizeof(glm::vec3) * MAX_LINE_VERTICES;
    mVertexPositionBuffer = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
        CD3DX12_RESOURCE_DESC::Buffer(bufferSize), nullptr, "Line vertex position buffer");
    mVertexPositionBuffer->CreateUploadBuffer(device, bufferSize, 0);

    mVertexPositionBufferView.BufferLocation = mVertexPositionBuffer->GetResource()->GetGPUVirtualAddress();
    mVertexPositionBufferView.StrideInBytes = sizeof(glm::vec3);
    mVertexPositionBufferView.SizeInBytes = bufferSize;

    bufferSize = sizeof(glm::vec4) * MAX_LINE_VERTICES;
    mVertexColorBuffer = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), 
        CD3DX12_RESOURCE_DESC::Buffer(bufferSize), nullptr, "Line vertex color buffer");
    mVertexColorBuffer->CreateUploadBuffer(device, bufferSize, 0);

    mVertexColorBufferView.BufferLocation = mVertexColorBuffer->GetResource()->GetGPUVirtualAddress();
    mVertexColorBufferView.StrideInBytes = sizeof(glm::vec4);
    mVertexColorBufferView.SizeInBytes = bufferSize;

    mPositions.resize(MAX_LINE_VERTICES);
    mColors.resize(MAX_LINE_VERTICES);
}

CE::DebugRenderingData::~DebugRenderingData() = default;

CE::UIRenderingData::UIRenderingData()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
    ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());
    engineDevice.StartUploadCommands();

    std::vector<glm::vec3> positions =
    {
        glm::vec3(-0.5f, 0.5f, 0.0f),  // Top Left
        glm::vec3(0.5f, 0.5f, 0.0f),   // Top Right
        glm::vec3(-0.5f, -0.5f, 0.0f), // Bottom Left
        glm::vec3(0.5f, -0.5f, 0.0f),  // Bottom Right
    };
    std::vector<glm::vec2> uvs = 
    {
        glm::vec2(0.f),
        glm::vec2(1.f, 0.f),
        glm::vec2(0.f, 1.f),
        glm::vec2(1.f, 1.f)
    };
    std::vector<uint32> indices = { 0, 1, 2, 3, 2, 1 };
    int vBufferSize = sizeof(glm::vec3) * static_cast<int>(positions.size());
    int tBufferSize = sizeof(glm::vec2) * static_cast<int>(uvs.size());
    int iBufferSize = sizeof(uint32) * static_cast<int>(indices.size());

    mQuadVResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), nullptr, "UI Vertex resource buffer");
    mQuadUVResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(tBufferSize), nullptr, "UI UV resource buffer");
    mIndicesResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), nullptr, "UI indices resource buffer");

    mQuadVResource->CreateUploadBuffer(device, vBufferSize, 0);
    mQuadUVResource->CreateUploadBuffer(device, tBufferSize, 0);
    mIndicesResource->CreateUploadBuffer(device, iBufferSize, 0);

    D3D12_SUBRESOURCE_DATA vData = {};
    vData.pData = positions.data();
    vData.RowPitch = sizeof(float) * 3;
    vData.SlicePitch = vBufferSize;

    D3D12_SUBRESOURCE_DATA uData = {};
    uData.pData = uvs.data();
    uData.RowPitch = sizeof(float) * 2;
    uData.SlicePitch = tBufferSize;

    D3D12_SUBRESOURCE_DATA iData = {};
    iData.pData = indices.data();
    iData.RowPitch = iBufferSize;
    iData.SlicePitch = iBufferSize;

    mQuadVResource->Update(uploadCmdList, vData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    mQuadUVResource->Update(uploadCmdList, uData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    mIndicesResource->Update(uploadCmdList, iData, D3D12_RESOURCE_STATE_INDEX_BUFFER, 0, 1);

    mVertexBufferView.BufferLocation = mQuadVResource->GetResource()->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(float) * 3;
    mVertexBufferView.SizeInBytes = vBufferSize;

    mTexCoordBufferView.BufferLocation = mQuadUVResource->GetResource()->GetGPUVirtualAddress();
    mTexCoordBufferView.StrideInBytes = sizeof(float) * 2;
    mTexCoordBufferView.SizeInBytes = tBufferSize;

    mIndexBufferView.BufferLocation = mIndicesResource->GetResource()->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mIndexBufferView.SizeInBytes = iBufferSize;

    mModelMatBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "UI Mesh matrix data", FRAME_BUFFER_COUNT);
    mColorBuffer = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::ColorInfo), MAX_MESHES, "UI Mesh color data", FRAME_BUFFER_COUNT);
    engineDevice.SubmitUploadCommands();
}

CE::PosProcRenderingData::PosProcRenderingData()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
    ID3D12GraphicsCommandList4* uploadCmdList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());
    engineDevice.StartUploadCommands();

    std::vector<glm::vec3> positions =
    {
        glm::vec3(-1.f, 1.f, 0.0f),  // Top Left
        glm::vec3(1.f, 1.f, 0.0f),   // Top Right
        glm::vec3(-1.f, -1.f, 0.0f), // Bottom Left
        glm::vec3(1.f, -1.f, 0.0f),  // Bottom Right
    };

    std::vector<glm::vec2> uvs = 
    {
        glm::vec2(0.f),
        glm::vec2(1.f, 0.f),
        glm::vec2(0.f, 1.f),
        glm::vec2(1.f, 1.f)
    };
    std::vector<uint32> indices = { 0, 1, 2, 3, 2, 1 };
    int vBufferSize = sizeof(glm::vec3) * static_cast<int>(positions.size());
    int tBufferSize = sizeof(glm::vec2) * static_cast<int>(uvs.size());
    int iBufferSize = sizeof(uint32) * static_cast<int>(indices.size());

    mQuadVResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), nullptr, "UI Vertex resource buffer");
    mQuadUVResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(tBufferSize), nullptr, "UI UV resource buffer");
    mIndicesResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), nullptr, "UI indices resource buffer");

    mQuadVResource->CreateUploadBuffer(device, vBufferSize, 0);
    mQuadUVResource->CreateUploadBuffer(device, tBufferSize, 0);
    mIndicesResource->CreateUploadBuffer(device, iBufferSize, 0);

    D3D12_SUBRESOURCE_DATA vData = {};
    vData.pData = positions.data();
    vData.RowPitch = sizeof(float) * 3;
    vData.SlicePitch = vBufferSize;

    D3D12_SUBRESOURCE_DATA uData = {};
    uData.pData = uvs.data();
    uData.RowPitch = sizeof(float) * 2;
    uData.SlicePitch = tBufferSize;

    D3D12_SUBRESOURCE_DATA iData = {};
    iData.pData = indices.data();
    iData.RowPitch = iBufferSize;
    iData.SlicePitch = iBufferSize;

    mQuadVResource->Update(uploadCmdList, vData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    mQuadUVResource->Update(uploadCmdList, uData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    mIndicesResource->Update(uploadCmdList, iData, D3D12_RESOURCE_STATE_INDEX_BUFFER, 0, 1);

    mVertexBufferView.BufferLocation = mQuadVResource->GetResource()->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(float) * 3;
    mVertexBufferView.SizeInBytes = vBufferSize;

    mTexCoordBufferView.BufferLocation = mQuadUVResource->GetResource()->GetGPUVirtualAddress();
    mTexCoordBufferView.StrideInBytes = sizeof(float) * 2;
    mTexCoordBufferView.SizeInBytes = tBufferSize;

    mIndexBufferView.BufferLocation = mIndicesResource->GetResource()->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mIndexBufferView.SizeInBytes = iBufferSize;

    mOutlineBuffer = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXOutlineInfo), 1, "Outline info", FRAME_BUFFER_COUNT);
    engineDevice.SubmitUploadCommands();

}

void CE::PosProcRenderingData::Update(const World& world)
{
    Device& engineDevice = Device::Get();
    int frameIndex = engineDevice.GetFrameIndex();

    {
        InfoStruct::DXOutlineInfo outlineInfo{};
        const auto view = world.GetRegistry().View<const PostPrOutlineComponent>();
        for (auto [entity, outlineComponent] : view.each())
        {
            outlineInfo.mOutlineColor = outlineComponent.mColor;
            outlineInfo.mThickness = outlineComponent.mThickness;
        }
        mOutlineBuffer->Update(&outlineInfo, sizeof(InfoStruct::DXOutlineInfo), 0, frameIndex);
    }
}

CE::GPUWorld::GPUWorld(const World& world)
    :
    IGPUWorld::IGPUWorld(world)
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
    mClusterGrid = glm::ivec3(16, 8, 24);
    mNumberOfClusters = mClusterGrid.x * mClusterGrid.y * mClusterGrid.z;

    // Create constant buffers
    mConstBuffers[InfoStruct::CAM_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 3, "Matrix buffer default shader", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::LIGHT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXLightInfo), 1, "Point light buffer", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::MATERIAL_INFO_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMaterialInfo), MAX_MESHES + 2, "Model material info", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::PARTICLE_MATERIAL_INFO_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMaterialInfo), MAX_PARTICLES, "Model material info", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "Mesh matrix data", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::PARTICLE_MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_PARTICLES, "Particle matrix data", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::FINAL_BONE_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * MAX_BONES, MAX_SKINNED_MESHES, "Skinned mesh bone matrices", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::COLOR_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXColorMultiplierInfo), MAX_MESHES, "Color multiplier", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::PARTICLE_COLOR_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXColorMultiplierInfo), MAX_PARTICLES, "Particle color multiplier", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::UI_MODEL_MAT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "UI MODEL MATRICES", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::CLUSTER_INFO_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::Clustering::DXCluster), 1, "Cluster creation data", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::CLUSTERING_CAM_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::Clustering::DXCameraClustering), 1, "Clustering camera data", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::FOG_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXFogInfo), 1, "Fog info", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::PARTICLE_INFO_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXParticleInfo), MAX_PARTICLES, "Particle info", FRAME_BUFFER_COUNT);
    
    // Create structured buffers
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXDirLightInfo) * 10, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "DIRECTIONAL LIGHT STRUCTURED BUFFER");
    mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXDirLightInfo) * 10, 0);
    mDirectionalLights.resize(10);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXPointLightInfo) * 100, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::POINT_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "POINT LIGHT STRUCTURED BUFFER");
    mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXPointLightInfo) * 100, 0);
    mPointLights.resize(100);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::Clustering::DXAABB) * mNumberOfClusters, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::CLUSTER_GRID_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "CLUSTER GRID BUFFER");

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::Clustering::DXLightGridElement) * mNumberOfClusters, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::LIGHT_GRID_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "LIGHT GRID BUFFER");
    mStructuredBuffers[InfoStruct::LIGHT_GRID_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::Clustering::DXLightGridElement) * mNumberOfClusters, 0);


    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::POINT_LIGHT_COUNTER] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "POINT LIGHT COUNTER BUFFER");
    mStructuredBuffers[InfoStruct::POINT_LIGHT_COUNTER]->CreateUploadBuffer(device, sizeof(uint32), 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(int32) * mNumberOfClusters * MAX_LIGHTS_PER_CLUSTER, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::LIGHT_INDICES] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "LIGHTI INDICES BUFFER");
    mStructuredBuffers[InfoStruct::LIGHT_INDICES]->CreateUploadBuffer(device, sizeof(int32) * mNumberOfClusters * MAX_LIGHTS_PER_CLUSTER, 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32) * mNumberOfClusters, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::ACTIVE_CLUSTER_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "ACTIVE CLUSTER BUFFER");
    mStructuredBuffers[InfoStruct::ACTIVE_CLUSTER_SB]->CreateUploadBuffer(device, sizeof(uint32) * mNumberOfClusters, 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * mNumberOfClusters, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "COMPACT CLUSTER BUFFER");
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB]->CreateUploadBuffer(device, sizeof(unsigned int) * mNumberOfClusters, 0);
    
    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "COMPACT CLUSTER COUNTER BUFFER");
    mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER]->CreateUploadBuffer(device, sizeof(unsigned int), 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int));
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_READBACK_RESOURCE] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "COMPACT CLUSTER COUNTER READBACK BUFFER", D3D12_RESOURCE_STATE_COPY_DEST);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::Clustering::DXAABB) * mNumberOfClusters);
    mStructuredBuffers[InfoStruct::GRID_READBACK_RESOURCE] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "READBACK GRID BUFFER", D3D12_RESOURCE_STATE_COPY_DEST);

    D3D12_SHADER_RESOURCE_VIEW_DESC  srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXPointLightInfo);
    srvDesc.Buffer.NumElements = 100;
    mPointLightsSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::POINT_LIGHT_SB].get(), &srvDesc);

    //Directional lights
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXDirLightInfo);
    srvDesc.Buffer.NumElements = 10;
    mDirectionalLightsSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB].get(), &srvDesc);

    //AABB Clusters
    srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXAABB);
    srvDesc.Buffer.NumElements = mNumberOfClusters;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mClusterSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::CLUSTER_GRID_SB].get(), &srvDesc);

    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXLightGridElement);
    mLightGridSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::LIGHT_GRID_SB].get(), &srvDesc);

    //Active clusters
    srvDesc.Buffer.StructureByteStride = sizeof(uint32);
    mActiveClusterSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::ACTIVE_CLUSTER_SB].get(), &srvDesc); 
    srvDesc.Buffer.StructureByteStride = sizeof(uint32);
    srvDesc.Buffer.NumElements = mNumberOfClusters * MAX_LIGHTS_PER_CLUSTER;
    mLightIndicesSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::LIGHT_INDICES].get(), &srvDesc); 

    //Compact cluster
    srvDesc.Buffer.NumElements = mNumberOfClusters;
    srvDesc.Buffer.StructureByteStride = sizeof(uint32);
    mCompactClusterSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB].get(), &srvDesc); 

    //CREATE UAVS
    //AABB Clusters
    D3D12_UNORDERED_ACCESS_VIEW_DESC  uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXAABB);
    uavDesc.Buffer.NumElements = mNumberOfClusters;
    mClusterUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::CLUSTER_GRID_SB].get(), &uavDesc);

    //Light grid
    uavDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXLightGridElement);
    mLightGridUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::LIGHT_GRID_SB].get(), &uavDesc);

    //Active clusters
    uavDesc.Buffer.StructureByteStride = sizeof(uint32);
    mActiveClusterUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::ACTIVE_CLUSTER_SB].get(), &uavDesc); 
    uavDesc.Buffer.StructureByteStride = sizeof(uint32);
    uavDesc.Buffer.NumElements = mNumberOfClusters * MAX_LIGHTS_PER_CLUSTER;
    mLightIndicesUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::LIGHT_INDICES].get(), &uavDesc); 

    //Compact clusters
    uavDesc.Buffer.NumElements = mNumberOfClusters;
    uavDesc.Buffer.StructureByteStride = sizeof(uint32);
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    mCompactClusterUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB].get(), &uavDesc, mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER].get()); 

    uavDesc.Buffer.NumElements = 1;
    mPointLightCounterUAVSlot =  engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::POINT_LIGHT_COUNTER].get(), &uavDesc); 

    InitializeShadowMaps();
    mSelectedMeshFrameBuffer = std::make_unique<FrameBuffer>(glm::vec2(1920, 1080));
    mParticles.resize(MAX_PARTICLES);
}

CE::GPUWorld::~GPUWorld() = default;

void CE::GPUWorld::Update()
{
    Device& engineDevice = Device::Get();
    int frameIndex = engineDevice.GetFrameIndex();

    entt::entity cameraOwner = CameraComponent::GetSelected(mWorld);

    if (cameraOwner == entt::null)
    {
        return;
    }

    const CameraComponent& camera = mWorld.get().GetRegistry().Get<const CameraComponent>(cameraOwner);
    const TransformComponent& cameraTransform = mWorld.get().GetRegistry().Get<const TransformComponent>(cameraOwner);
    // Update camera
    InfoStruct::DXMatrixInfo matrixInfo{};
    matrixInfo.pm = glm::transpose(camera.GetProjection());
    matrixInfo.vm = glm::transpose(camera.GetView());

    matrixInfo.ipm = glm::inverse(matrixInfo.pm);
    matrixInfo.ivm = glm::inverse(matrixInfo.vm);
    mConstBuffers[InfoStruct::CAM_MATRIX_CB]->Update(&matrixInfo, sizeof(InfoStruct::DXMatrixInfo), 0, frameIndex);

    // Update lights
    const auto pointLightView = mWorld.get().GetRegistry().View<const PointLightComponent, const TransformComponent>();
    const auto dirLightView = mWorld.get().GetRegistry().View<const DirectionalLightComponent, const TransformComponent>();
    int dirLightCounter = 0;
    mPointLightCounter = 0;

    for (auto [entity, lightComponent, transform] : pointLightView.each()) {
        if(mPointLightCounter >= mPointLights.size())
        {
            mPointLights.resize(mPointLights.size() + 100);
            mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->mResizeBuffer = true;
        }

        InfoStruct::DXPointLightInfo pointLight;
        pointLight.mPosition = glm::vec4(transform.GetWorldPosition(),1.f);
        pointLight.mColorAndIntensity = glm::vec4(lightComponent.mColor, lightComponent.mIntensity);
        pointLight.mRadius = lightComponent.mRange;
        mPointLights[mPointLightCounter] = pointLight;
        mPointLightCounter++;
    }

    mLightInfo.mActiveShadowingLight = -1;
    for (auto [entity, lightComponent, transform] : dirLightView.each()) {

        if(dirLightCounter >= mDirectionalLights.size())
        {
            mDirectionalLights.resize(mDirectionalLights.size() + 10);
            mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB]->mResizeBuffer = true;
        }

        glm::quat quatRotation = transform.GetLocalOrientation();
        glm::vec3 baseDir = glm::vec3(0, 0, 1);
        glm::vec3 lightDirection = quatRotation * baseDir;
        float extent = lightComponent.mShadowExtent;

        glm::mat4x4 projection = glm::orthoLH_ZO(-extent, extent, -extent, extent, -extent, extent);
        glm::mat4x4 view = lightComponent.GetShadowView(mWorld, transform);
        
        InfoStruct::DXMatrixInfo lightCameraMap;
        lightCameraMap.pm = glm::transpose(projection);
        lightCameraMap.vm = glm::transpose(view);

        // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
        glm::mat4x4 t(
            0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, -0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f);

        InfoStruct::DXDirLightInfo dirLight;
        dirLight.mDir = glm::vec4(lightDirection, 1.f);
        dirLight.mColorAndIntensity = glm::vec4(lightComponent.mColor, lightComponent.mIntensity);
        dirLight.mLightMat = glm::transpose(t*projection*view);
        dirLight.mBias = lightComponent.mShadowBias;
        dirLight.mCastsShadows = lightComponent.mCastShadows;
        dirLight.mNumSamples = lightComponent.mShadowSamples;
        mDirectionalLights[dirLightCounter] = dirLight;

        mConstBuffers[InfoStruct::CAM_MATRIX_CB]->Update(&lightCameraMap, sizeof(InfoStruct::DXMatrixInfo), 1, frameIndex);

        if (lightComponent.mCastShadows)
            mLightInfo.mActiveShadowingLight = dirLightCounter;

        dirLightCounter++;
    }

    UpdateParticles(cameraTransform.GetLocalPosition());
    UpdateLights(dirLightCounter, mPointLightCounter);

    // Update materials
    // 
    // I'm not sure why, because I (Guus), know nothing of dx12, but dx12 does not like it
    // if we are sending textures to the GPU in the RENDERING pass. Soo my solution was to
    // do it here instead. The only downside is that we have to iterate over the static meshes
    // twice, but in entt this is surprisingly fast.
    int meshCounter = 0;
    {
        // We get transform to make sure the mesh count is correct, since the user can have a mesh without a transform
        const auto view = mWorld.get().GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();

        for (auto [entity, skinnedMeshComponent, transformComponent] : view.each())
        {
            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                meshCounter++;
                continue;
            }

            // Update material
            InfoStruct::DXMaterialInfo materialInfo = GetMaterial(skinnedMeshComponent.mMaterial.Get());
            mConstBuffers[InfoStruct::MATERIAL_INFO_CB]->Update(&materialInfo, sizeof(InfoStruct::DXMaterialInfo), meshCounter, frameIndex);

            glm::mat4x4 modelMatrices[2]{};
            modelMatrices[0] = glm::transpose(transformComponent.GetWorldMatrix());
            modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
            mConstBuffers[InfoStruct::MODEL_MATRIX_CB]->Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);

            // Do we now have a loot of unused space?
            // If we first draw 10'000 static meshes, then meshCounter will be 10'000,
            // and the first 10'000 FINAL_BONE_MATRIX_CB slots will be unused, with
            // each slot being quite large. This leads to buffer overflows with many
            // static meshes - Guus
            const auto& boneMatrices = skinnedMeshComponent.mFinalBoneMatrices;
            mConstBuffers[InfoStruct::FINAL_BONE_MATRIX_CB]->Update(boneMatrices.data(), boneMatrices.size() * sizeof(glm::mat4x4), meshCounter, frameIndex);

            meshCounter++;
        }
    }

    {
        // We get transform to make sure the mesh count is correct, since the user can have a mesh without a transform
        const auto view = mWorld.get().GetRegistry().View<const StaticMeshComponent, const TransformComponent>();

        for (auto [entity, staticMeshComponent, transformComponent] : view.each())
        {
            if (!staticMeshComponent.mStaticMesh)
            {
                meshCounter++;
                continue;
            }

            InfoStruct::DXMaterialInfo materialInfo = GetMaterial(staticMeshComponent.mMaterial.Get());
            float uvScale = staticMeshComponent.mTiling;

            if(staticMeshComponent.mTilesWithMeshScale)
            {
                float meshScale = transformComponent.GetWorldScaleUniform();
                float scaleFactor = meshScale < 1 && meshScale >0 ? 1 / meshScale : meshScale;
                uvScale *= scaleFactor;
            }

            materialInfo.uvScale = glm::vec4(uvScale, uvScale, 1.f, 1.f);

            mConstBuffers[InfoStruct::MATERIAL_INFO_CB]->Update(&materialInfo, sizeof(InfoStruct::DXMaterialInfo), meshCounter, frameIndex);

            glm::mat4x4 modelMatrices[2]{};
            modelMatrices[0] = glm::transpose(transformComponent.GetWorldMatrix());
            modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
            mConstBuffers[InfoStruct::MODEL_MATRIX_CB]->Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);

            meshCounter++;
        }
    }


    {
        InfoStruct::DXFogInfo fog{};
        const auto view = mWorld.get().GetRegistry().View<const FogComponent>();
        for (auto [entity, fogComponent] : view.each())
        {
            fog.mApplyFog = true;
            fog.mColor = fogComponent.mColor;
            fog.mFarPlane = fogComponent.mFarPlane;
            fog.mNearPlane = fogComponent.mNearPlane;
        }
        mConstBuffers[InfoStruct::FOG_CB]->Update(&fog, sizeof(InfoStruct::DXFogInfo), 0, frameIndex);
    }

    InfoStruct::DXMatrixInfo UIcamera;
    UIcamera.pm = glm::transpose(camera.GetOrthographicProjection());
    mConstBuffers[InfoStruct::CAM_MATRIX_CB]->Update(&UIcamera, sizeof(InfoStruct::DXMatrixInfo), 2, frameIndex);

    mPostProcData.Update(mWorld);
    UpdateClusterData(camera);
}

uint32 CE::GPUWorld::ReadCompactClusterCounter() const
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    D3D12_RESOURCE_STATES prevState = mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER]->GetState();

    mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER]->ChangeState(commandList, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->CopyResource(mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_READBACK_RESOURCE]->Get(), mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER]->Get());

    mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER]->ChangeState(commandList, prevState);

    void* mappedData;
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_READBACK_RESOURCE]->Get()->Map(0, nullptr, &mappedData);

    uint32_t value = *static_cast<uint32_t*>(mappedData);

    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_READBACK_RESOURCE]->Get()->Unmap(0, nullptr);

    return value;
}

void CE::GPUWorld::InitializeShadowMaps()
{
    mShadowMap = std::make_unique<InfoStruct::DXShadowMapInfo>();
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, 4096, 4096, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    mShadowMap->mDepthResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), resourceDesc, &depthOptimizedClearValue, "DIRECTIONAL LIGHT DEPTH STENCIL");

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
    mShadowMap->mDepthHandle = engineDevice.GetDescriptorHeap(DEPTH_HEAP)->AllocateDepthStencil(mShadowMap->mDepthResource.get(), &depthStencilDesc);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    mShadowMap->mDepthSRVHandle = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mShadowMap->mDepthResource.get(), &srvDesc);

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Use the format that matches your RTV format.
    clearValue.Color[0] = 0.f; // Red component
    clearValue.Color[1] = 0.f; // Green component
    clearValue.Color[2] = 0.f; // Blue component
    clearValue.Color[3] = 0.f; // Alpha component
    resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4096, 4096, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    mShadowMap->mRenderTarget = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), resourceDesc, &clearValue, "DIRECTIONAL LIGHT RENDER TARGET");

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    mShadowMap-> mRTHandle = engineDevice.GetDescriptorHeap(RT_HEAP)->AllocateRenderTarget(mShadowMap->mRenderTarget.get(), &rtvDesc);

    mShadowMap->mViewport.Width = static_cast<FLOAT>(4096);
    mShadowMap->mViewport.Height = static_cast<FLOAT>(4096);
    mShadowMap->mViewport.TopLeftX = 0;
    mShadowMap->mViewport.TopLeftY = 0;
    mShadowMap->mViewport.MinDepth = 0.0f;
    mShadowMap->mViewport.MaxDepth = 1.0f;

    mShadowMap->mScissorRect.left = 0;
    mShadowMap->mScissorRect.top = 0;
    mShadowMap->mScissorRect.right = static_cast<LONG>(mShadowMap->mViewport.Width);
    mShadowMap->mScissorRect.bottom = static_cast<LONG>(mShadowMap->mViewport.Height);
}

void CE::GPUWorld::UpdateParticles(glm::vec3 cameraPos)
{
    Device& engineDevice = Device::Get();
    int frameIndex = engineDevice.GetFrameIndex();

    const auto simpleColorParticles = mWorld.get().GetRegistry().View<const ParticleEmitterComponent, const ParticleMeshRendererComponent, const ParticleColorComponent>(entt::exclude<ParticleColorOverTimeComponent>);
    const auto changingColorParticles = mWorld.get().GetRegistry().View<const ParticleEmitterComponent, const ParticleMeshRendererComponent, const ParticleColorComponent, const ParticleColorOverTimeComponent>();

    mParticleCount = 0;

    {
        for (auto [entity, emitter, meshRenderer, colorComponent] : simpleColorParticles.each())
        {
            bool emitterPlaying = emitter.IsPlaying();
            bool meshPresent = meshRenderer.mParticleMesh;
            if (!emitterPlaying || !meshPresent)
            {
                continue;
            }

            const size_t numOfParticles = emitter.GetNumOfParticles();

            Span<const glm::vec3> positions = emitter.GetParticlePositions();
            Span<const glm::vec3> sizes = emitter.GetParticleSizes();
            Span<const glm::quat> orientations = emitter.GetParticleOrientations();
            Span<const LinearColor> colors = colorComponent.GetColors();
            auto lightComponent = mWorld.get().GetRegistry().TryGet<ParticleLightComponent>(entity);
            Span<const float> intensities;
            if (lightComponent)
                intensities = lightComponent->GetParticleLightIntensities();

            for (uint32 i = 0; i < numOfParticles; i++)
            {
                if (!emitter.IsParticleAlive(i))
                    continue;

                if (mParticleCount >= MAX_PARTICLES)
                {
                    LOG(LogCore, Warning, "Maximum of particles per frame reached. Tell a programmer to increase it :)");
                    return;
                }

                const glm::mat4 mat = TransformComponent::ToMatrix(positions[i], sizes[i], orientations[i]);

                InfoStruct::DXParticleInfo particleInfo{};
                particleInfo.mMesh = const_cast<StaticMesh*>(meshRenderer.mParticleMesh.Get());
                particleInfo.mMaterial = const_cast<Material*>(meshRenderer.mParticleMaterial.Get()); 
                if(meshRenderer.mParticleMaterial)
                    particleInfo.mMaterialInfo = GetMaterial(meshRenderer.mParticleMaterial.Get());
                particleInfo.mDistanceToCamera = glm::length(positions[i] - cameraPos);
                particleInfo.mColor = colors[i];
                particleInfo.mMatrix = std::move(mat);

                if (lightComponent)
                {
                    particleInfo.mIsEmissive = true;
                    particleInfo.mLightRadius = lightComponent->mLightRadius;
                    particleInfo.mLightIntensity = intensities[i];

                    if(mPointLightCounter >= mPointLights.size())
                    {
                        mPointLights.resize(mPointLights.size() + 100);
                        mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->mResizeBuffer = true;
                    }

                    InfoStruct::DXPointLightInfo pointLight;
                    pointLight.mPosition = glm::vec4(positions[i],1.f);
                    pointLight.mColorAndIntensity = glm::vec4(glm::vec3(particleInfo.mColor), particleInfo.mLightIntensity);
                    pointLight.mRadius = lightComponent->mLightRadius;
                    mPointLights[mPointLightCounter] = pointLight;
                    mPointLightCounter++;
                }

                mParticles[mParticleCount] = std::move(particleInfo);

                mParticleCount++;
            }          
        }
    }

    {

        for (auto [entity, emitter, meshRenderer, colorComponent, colorOverTime] : changingColorParticles.each())
        {
            bool emitterPlaying = emitter.IsPlaying();
            bool meshPresent = meshRenderer.mParticleMesh;
            if (!emitterPlaying || !meshPresent)
            {
                continue;
            }

            const size_t numOfParticles = emitter.GetNumOfParticles();

            Span<const float> lifeTimes = emitter.GetParticleLifeTimesAsPercentage();
            Span<const glm::vec3> positions = emitter.GetParticlePositions();
            Span<const glm::vec3> sizes = emitter.GetParticleSizes();
            Span<const glm::quat> orientations = emitter.GetParticleOrientations();
            Span<const LinearColor> colors = colorComponent.GetColors();
            const ColorGradient& gradient = colorOverTime.mGradient;
            auto lightComponent = mWorld.get().GetRegistry().TryGet<ParticleLightComponent>(entity);
            Span<const float> intensities;
            if (lightComponent)
                intensities = lightComponent->GetParticleLightIntensities();

            for (uint32 i = 0; i < numOfParticles; i++)
            {
                if (!emitter.IsParticleAlive(i))
                    continue;

                if (mParticleCount >= MAX_PARTICLES)
                {
                    LOG(LogCore, Warning, "Maximum of particles per frame reached. Tell a programmer to increase it :)");
                    return;
                }

                const glm::mat4 mat = TransformComponent::ToMatrix(positions[i], sizes[i], orientations[i]);

                InfoStruct::DXParticleInfo particleInfo{};
                particleInfo.mMesh = const_cast<StaticMesh*>(meshRenderer.mParticleMesh.Get());
                particleInfo.mMaterial = const_cast<Material*>(meshRenderer.mParticleMaterial.Get()); 
                if(meshRenderer.mParticleMaterial)
                    particleInfo.mMaterialInfo = GetMaterial(meshRenderer.mParticleMaterial.Get());
                particleInfo.mDistanceToCamera = glm::length(positions[i] - cameraPos);
                particleInfo.mColor = colors[i] * gradient.GetColorAt(lifeTimes[i]);
                particleInfo.mMatrix = std::move(mat);
                if (lightComponent)
                {
                    particleInfo.mIsEmissive = true;
                    particleInfo.mLightRadius = lightComponent->mLightRadius;
                    particleInfo.mLightIntensity = intensities[i];

                    if(mPointLightCounter >= mPointLights.size())
                    {
                        mPointLights.resize(mPointLights.size() + 100);
                        mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->mResizeBuffer = true;
                    }

                    InfoStruct::DXPointLightInfo pointLight;
                    pointLight.mPosition = glm::vec4(positions[i],1.f);
                    pointLight.mColorAndIntensity = glm::vec4(glm::vec3(particleInfo.mColor), particleInfo.mLightIntensity);
                    pointLight.mRadius = lightComponent->mLightRadius;
                    mPointLights[mPointLightCounter] = pointLight;
                    mPointLightCounter++;
                }

                mParticles[mParticleCount] = std::move(particleInfo);

                mParticleCount++;
            }
        }
    }

    for (int i = 0; i < mParticleCount; i++) {
        mConstBuffers[InfoStruct::PARTICLE_MATERIAL_INFO_CB]->Update(&mParticles[i].mMaterialInfo, sizeof(InfoStruct::DXMaterialInfo), i, frameIndex);
        glm::mat4x4 modelMatrices[2]{};
        modelMatrices[0] = glm::transpose(mParticles[i].mMatrix);
        modelMatrices[1] = glm::transpose(glm::inverse(mParticles[i].mMatrix));
        mConstBuffers[InfoStruct::PARTICLE_MODEL_MATRIX_CB]->Update(modelMatrices, sizeof(glm::mat4x4) * 2, i, frameIndex);
        mConstBuffers[InfoStruct::PARTICLE_COLOR_CB]->Update(&mParticles[i].mColor, sizeof(InfoStruct::DXColorMultiplierInfo), i, frameIndex);
        
        InfoStruct::DXParticleBufferInfo particleInfo{};
        particleInfo.mIsEmissive = mParticles[i].mIsEmissive;
        particleInfo.mEmissionIntensity = mParticles[i].mLightIntensity;
        mConstBuffers[InfoStruct::PARTICLE_INFO_CB]->Update(&particleInfo, sizeof(InfoStruct::DXParticleBufferInfo), i, frameIndex);
    }
}

CE::InfoStruct::DXMaterialInfo CE::GPUWorld::GetMaterial(const CE::Material* material)
{
    InfoStruct::DXMaterialInfo materialInfo{};

    if (material != nullptr) 
    {
        materialInfo.colorFactor = { material->mBaseColorFactor.r,
            material->mBaseColorFactor.g,
            material->mBaseColorFactor.b,
            material->mBaseColorFactor.a };

        materialInfo.emissiveFactor = { material->mEmissiveFactor.r,
            material->mEmissiveFactor.g,
            material->mEmissiveFactor.b,
            1.f};

        materialInfo.metallicFactor = material->mMetallicFactor;
        materialInfo.roughnessFactor = material->mRoughnessFactor;
        materialInfo.normalScale = material->mNormalScale;

        materialInfo.useColorTex = material->mBaseColorTexture != nullptr;
        materialInfo.useEmissiveTex = material->mEmissiveTexture != nullptr;
        materialInfo.useMetallicRoughnessTex = material->mMetallicRoughnessTexture != nullptr;
        materialInfo.useNormalTex = material->mNormalTexture != nullptr;
        materialInfo.useOcclusionTex = material->mOcclusionTexture != nullptr;
    }
    else 
    {
        materialInfo.colorFactor = { 1.f, 1.f, 1.f, 1.f };
        materialInfo.emissiveFactor = { 1.f, 1.f, 1.f, 1.f };
        materialInfo.metallicFactor = 0.f;
        materialInfo.roughnessFactor = 0.f;
        materialInfo.normalScale = 1.f;
        materialInfo.useColorTex = false;
        materialInfo.useEmissiveTex = false;
        materialInfo.useMetallicRoughnessTex = false;
        materialInfo.useNormalTex = false;
        materialInfo.useOcclusionTex = false;
    }

    return materialInfo;
}

void CE::GPUWorld::UpdateClusterData(const CameraComponent& camera)
{
    Device& engineDevice = Device::Get();
    int frameIndex = engineDevice.GetFrameIndex();

    InfoStruct::Clustering::DXCluster clusterInfo;
    clusterInfo.mNumClustersX = mClusterGrid.x;
    clusterInfo.mNumClustersY = mClusterGrid.y;
    clusterInfo.mNumClustersZ = mClusterGrid.z;
    clusterInfo.mMaxLightsInCluster = MAX_LIGHTS_PER_CLUSTER;
    mConstBuffers[InfoStruct::CLUSTER_INFO_CB]->Update(&clusterInfo, sizeof(InfoStruct::Clustering::DXCluster), 0, frameIndex);

    InfoStruct::Clustering::DXCameraClustering clusteringCam;
    clusteringCam.mFarPlane = camera.mFar;
    clusteringCam.mNearPlane = camera.mNear;
    clusteringCam.mDepthSliceScale = (float)clusterInfo.mNumClustersZ / std::log2f(clusteringCam.mFarPlane / clusteringCam.mNearPlane);
    clusteringCam.mDepthSliceBias = -((float)clusterInfo.mNumClustersZ * std::log2f(clusteringCam.mNearPlane) / std::log2f(clusteringCam.mFarPlane / clusteringCam.mNearPlane));
    clusteringCam.mLinearDepthCoefficient.x = clusteringCam.mFarPlane / (clusteringCam.mNearPlane - clusteringCam.mFarPlane);
    clusteringCam.mLinearDepthCoefficient.y = (clusteringCam.mNearPlane * clusteringCam.mFarPlane) / (clusteringCam.mNearPlane - clusteringCam.mFarPlane);
#ifdef EDITOR
    clusteringCam.mScreenDimensions = ImGui::GetContentRegionAvail();
#else
    clusteringCam.mScreenDimensions = engineDevice.GetDisplaySize();
#endif
    clusteringCam.mTileSize = glm::vec2(clusteringCam.mScreenDimensions.x / clusterInfo.mNumClustersX, clusteringCam.mScreenDimensions.y / clusterInfo.mNumClustersY);
    mConstBuffers[InfoStruct::CLUSTERING_CAM_CB]->Update(&clusteringCam, sizeof(InfoStruct::Clustering::DXCameraClustering), 0, frameIndex);
}

void CE::GPUWorld::ClearClusterData()
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

    // This uses a loot of memory and is quite slow,
    // is there not a DX equivalent for memset? - Guus
    const size_t zeroBufferSize = std::max(mNumberOfClusters * sizeof(uint32), mNumberOfClusters * sizeof(InfoStruct::Clustering::DXLightGridElement));
    static std::vector<uint8> manyZeroes(zeroBufferSize, 0);
	manyZeroes.resize(zeroBufferSize, 0);

    D3D12_SUBRESOURCE_DATA data;
    data.pData = manyZeroes.data();
    data.RowPitch = sizeof(uint32);
    data.SlicePitch = sizeof(uint32) * mNumberOfClusters;
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    static std::vector<int32> lightIndicesClear(mNumberOfClusters*MAX_LIGHTS_PER_CLUSTER, -1);
    data.pData = lightIndicesClear.data();
    data.RowPitch = sizeof(int32);
    data.SlicePitch = sizeof(int32) * mNumberOfClusters * MAX_LIGHTS_PER_CLUSTER;
    mStructuredBuffers[InfoStruct::LIGHT_INDICES]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    uint32 counterValue = 0;
    data.pData = &counterValue;
    data.RowPitch = sizeof(uint32);
    data.SlicePitch = sizeof(uint32);
    mStructuredBuffers[InfoStruct::POINT_LIGHT_COUNTER]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);
    mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    data.pData = manyZeroes.data();
    data.RowPitch = sizeof(InfoStruct::Clustering::DXLightGridElement);
    data.SlicePitch = sizeof(InfoStruct::Clustering::DXLightGridElement) * mNumberOfClusters;
    mStructuredBuffers[InfoStruct::LIGHT_GRID_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);
}

void CE::GPUWorld::UpdateLights(int numDirLights, int numPointLights)
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int frameIndex = engineDevice.GetFrameIndex();
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_SHADER_RESOURCE_VIEW_DESC  srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    mLightInfo.numDirLights = numDirLights;
    mLightInfo.numPointLights = numPointLights;
    mConstBuffers[InfoStruct::LIGHT_CB]->Update(&mLightInfo, sizeof(InfoStruct::DXLightInfo), 0, frameIndex);

    if (mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB]->mResizeBuffer)
    {
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXDirLightInfo) * static_cast<UINT>(mDirectionalLights.size()), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "DIRECTIONAL LIGHT STRUCTURED BUFFER");
        mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXDirLightInfo) * static_cast<UINT>(mDirectionalLights.size()), 0);
        srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXDirLightInfo);
        srvDesc.Buffer.NumElements =static_cast<UINT>(mDirectionalLights.size());
        mDirectionalLightsSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB].get(), &srvDesc);
        mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB]->mResizeBuffer = false;
    }

    if (mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->mResizeBuffer) {
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXPointLightInfo) * static_cast<UINT>(mPointLights.size()), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        mStructuredBuffers[InfoStruct::POINT_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "POINT LIGHT STRUCTURED BUFFER");
        mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXPointLightInfo) *static_cast<UINT>(mPointLights.size()), 0);
        srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXPointLightInfo);
        srvDesc.Buffer.NumElements = static_cast<UINT>(mPointLights.size());
        mPointLightsSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::POINT_LIGHT_SB].get(), &srvDesc);
        mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->mResizeBuffer = false;
    }

    D3D12_SUBRESOURCE_DATA data;
    data.pData = mDirectionalLights.data();
    data.RowPitch = sizeof(InfoStruct::DXDirLightInfo);
    data.SlicePitch = sizeof(InfoStruct::DXDirLightInfo) * numDirLights;
    mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    data.pData = mPointLights.data();
    data.RowPitch = sizeof(InfoStruct::DXPointLightInfo);
    data.SlicePitch = sizeof(InfoStruct::DXPointLightInfo) * numPointLights;
    mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);
}


