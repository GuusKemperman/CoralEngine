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
#include "Rendering/GPUWorld.h"

#include "Platform/PC/Core/DevicePC.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"

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

CE::GPUWorld::GPUWorld(const World& world)
    :
    IGPUWorld::IGPUWorld(world)
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    // Create constant buffers
    mConstBuffers[InfoStruct::CAM_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 1, "Matrix buffer default shader", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::LIGHT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXLightInfo), 1, "Point light buffer", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::MATERIAL_INFO_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMaterialInfo), MAX_MESHES + 2, "Model material info", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "Mesh matrix data", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::FINAL_BONE_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * MAX_BONES, MAX_SKINNED_MESHES, "Skinned mesh bone matrices", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::COLOR_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXColorMultiplierInfo), MAX_MESHES, "Color multiplier", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::UI_MODEL_MAT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "UI MODEL MATRICES", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::CLUSTER_INFO_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::Clustering::DXCluster), 1, "Cluster creation data", FRAME_BUFFER_COUNT);
    mConstBuffers[InfoStruct::CLUSTERING_CAM_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::Clustering::DXCameraClustering), 1, "Clustering camera data", FRAME_BUFFER_COUNT);
    
    // Create structured buffers
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXDirLightInfo) * 100, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "DIRECTIONAL LIGHT STRUCTURED BUFFER");
    mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXDirLightInfo) * 100, 0);
    mDirectionalLights.resize(100);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXPointLightInfo) * 100, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::POINT_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "POINT LIGHT STRUCTURED BUFFER");
    mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXPointLightInfo) * 100, 0);
    mPointLights.resize(100);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::Clustering::DXAABB) * 4000, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::CLUSTER_GRID_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "CLUSTER GRID BUFFER");

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::Clustering::DXLightGridElement) * 4000, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::LIGHT_GRID_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "LIGHT GRID BUFFER");
    mStructuredBuffers[InfoStruct::LIGHT_GRID_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::Clustering::DXLightGridElement) * 4000, 0);


    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::POINT_LIGHT_COUNTER] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "POINT LIGHT COUNTER BUFFER");
    mStructuredBuffers[InfoStruct::POINT_LIGHT_COUNTER]->CreateUploadBuffer(device, sizeof(uint32), 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(int32) * 4000, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::LIGHT_INDICES] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "LIGHTI INDICES BUFFER");
    mStructuredBuffers[InfoStruct::LIGHT_INDICES]->CreateUploadBuffer(device, sizeof(int32) * 4000, 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32) * 4000, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::ACTIVE_CLUSTER_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "ACTIVE CLUSTER BUFFER");
    mStructuredBuffers[InfoStruct::ACTIVE_CLUSTER_SB]->CreateUploadBuffer(device, sizeof(uint32) * 4000, 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * 4000, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "COMPACT CLUSTER BUFFER");
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB]->CreateUploadBuffer(device, sizeof(unsigned int) * 4000, 0);
    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "COMPACT CLUSTER COUNTER BUFFER");
    mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER]->CreateUploadBuffer(device, sizeof(unsigned int), 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int));
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_READBACK_RESOURCE] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "COMPACT CLUSTER COUNTER READBACK BUFFER", D3D12_RESOURCE_STATE_COPY_DEST);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::Clustering::DXAABB) * 4000);
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
    mDirectionalLightsSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB].get(), &srvDesc);

    //AABB Clusters
    srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXAABB);
    srvDesc.Buffer.NumElements = 4000;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mClusterSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::CLUSTER_GRID_SB].get(), &srvDesc);

    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXLightGridElement);
    mLightGridSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::LIGHT_GRID_SB].get(), &srvDesc);

    //Active clusters
    srvDesc.Buffer.StructureByteStride = sizeof(uint32);
    mActiveClusterSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::ACTIVE_CLUSTER_SB].get(), &srvDesc); 
    srvDesc.Buffer.StructureByteStride = sizeof(int32);
    mLightIndicesSRVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateResource(mStructuredBuffers[InfoStruct::LIGHT_INDICES].get(), &srvDesc); 

    //Compact clusters
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
    uavDesc.Buffer.NumElements = 4000;
    mClusterUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::CLUSTER_GRID_SB].get(), &uavDesc);

    //Light grid
    uavDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXLightGridElement);
    mLightGridUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::LIGHT_GRID_SB].get(), &uavDesc);

    //Active clusters
    uavDesc.Buffer.StructureByteStride = sizeof(uint32);
    mActiveClusterUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::ACTIVE_CLUSTER_SB].get(), &uavDesc); 
    uavDesc.Buffer.StructureByteStride = sizeof(int32);
    mLightIndicesUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::LIGHT_INDICES].get(), &uavDesc); 

    //Compact clusters
    uavDesc.Buffer.StructureByteStride = sizeof(uint32);
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    mCompactClusterUAVSlot = engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB].get(), &uavDesc, mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER].get()); 

    uavDesc.Buffer.NumElements = 1;
    mPointLightCounterUAVSlot =  engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->AllocateUAV(mStructuredBuffers[InfoStruct::POINT_LIGHT_COUNTER].get(), &uavDesc); 

    mClusterGrid = glm::ivec3(16, 8, 24);
}

CE::GPUWorld::~GPUWorld() = default;

void CE::GPUWorld::Update()
{
    Device& engineDevice = Device::Get();
    int frameIndex = engineDevice.GetFrameIndex();

    // Get main camera
    entt::entity cameraOwner = CameraComponent::GetSelected(mWorld);

    if (cameraOwner == entt::null)
    {
        return;
    }

    const CameraComponent& camera = mWorld.get().GetRegistry().Get<const CameraComponent>(cameraOwner);

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
    int pointLightCounter = 0;
    int dirLightCounter = 0;

    for (auto [entity, lightComponent, transform] : pointLightView.each()) {
        if(pointLightCounter >= mPointLights.size())
        {
            mPointLights.resize(mPointLights.size() + 100);
            mStructuredBuffers[InfoStruct::POINT_LIGHT_SB]->mResizeBuffer = true;
        }

        InfoStruct::DXPointLightInfo pointLight;
        pointLight.mPosition = glm::vec4(transform.GetWorldPosition(),1.f);
        pointLight.mColorAndIntensity = glm::vec4(lightComponent.mColor, lightComponent.mIntensity);
        pointLight.mRadius = lightComponent.mRange;
        mPointLights[pointLightCounter] = pointLight;
        pointLightCounter++;


    }

    for (auto [entity, lightComponent, transform] : dirLightView.each()) {

        if(dirLightCounter >= mDirectionalLights.size())
        {
            mDirectionalLights.resize(mDirectionalLights.size() + 100);
            mStructuredBuffers[InfoStruct::DIRECTIONAL_LIGHT_SB]->mResizeBuffer = true;
        }

        glm::quat quatRotation = transform.GetLocalOrientation();
        glm::vec3 baseDir = glm::vec3(0, 0, 1);
        glm::vec3 lightDirection = quatRotation * baseDir;

        InfoStruct::DXDirLightInfo dirLight;
        dirLight.mDir = glm::vec4(lightDirection, 1.f);
        dirLight.mColorAndIntensity = glm::vec4(lightComponent.mColor, lightComponent.mIntensity);
        mDirectionalLights[dirLightCounter] = dirLight;

        dirLightCounter++;
    }

    UpdateLights(dirLightCounter, pointLightCounter);

    // Update materials
    // 
    // I'm not sure why, because I (Guus), know nothing of dx12, but dx12 does not like it
    // if we are sending textures to the GPU in the RENDERING pass. Soo my solution was to
    // do it here instead. The only downside is that we have to iterate over the static meshes
    // twice, but in entt this is surprisingly fast.
    int meshCounter = 0;

    {
        // We get transform to make sure the mesh count is correct, since the user can have a mesh without a transform
        const auto view = mWorld.get().GetRegistry().View<const StaticMeshComponent, const TransformComponent>();

        for (auto [entity, staticMeshComponent, transformComponent] : view.each())
        {
            if (!staticMeshComponent.mStaticMesh)
            {
                continue;
            }

            InfoStruct::DXMaterialInfo materialInfo{};
            if (staticMeshComponent.mMaterial != nullptr)
            {
                SendMaterialTexturesToGPU(*staticMeshComponent.mMaterial);

                materialInfo.colorFactor = { staticMeshComponent.mMaterial->mBaseColorFactor.r,
                    staticMeshComponent.mMaterial->mBaseColorFactor.g,
                    staticMeshComponent.mMaterial->mBaseColorFactor.b,
                    0.f };

                materialInfo.emissiveFactor = { staticMeshComponent.mMaterial->mEmissiveFactor.r,
                    staticMeshComponent.mMaterial->mEmissiveFactor.g,
                    staticMeshComponent.mMaterial->mEmissiveFactor.b,
                    0.f };

                materialInfo.metallicFactor = staticMeshComponent.mMaterial->mMetallicFactor;
                materialInfo.roughnessFactor = staticMeshComponent.mMaterial->mRoughnessFactor;
                materialInfo.normalScale = staticMeshComponent.mMaterial->mNormalScale;
                materialInfo.useColorTex = staticMeshComponent.mMaterial->mBaseColorTexture != nullptr && staticMeshComponent.mMaterial->mBaseColorTexture->WasSentToGpu();
                materialInfo.useEmissiveTex = staticMeshComponent.mMaterial->mEmissiveTexture != nullptr && staticMeshComponent.mMaterial->mEmissiveTexture->WasSentToGpu();
                materialInfo.useMetallicRoughnessTex = staticMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr && staticMeshComponent.mMaterial->mMetallicRoughnessTexture->WasSentToGpu();
                materialInfo.useNormalTex = staticMeshComponent.mMaterial->mNormalTexture != nullptr && staticMeshComponent.mMaterial->mNormalTexture->WasSentToGpu();
                materialInfo.useOcclusionTex = staticMeshComponent.mMaterial->mOcclusionTexture != nullptr && staticMeshComponent.mMaterial->mOcclusionTexture->WasSentToGpu();
            }
            else 
            {
                materialInfo.colorFactor = { 0.f, 0.f, 0.f, 0.f };
                materialInfo.emissiveFactor = { 0.f, 0.f, 0.f, 0.f };
                materialInfo.metallicFactor = 0.f;
                materialInfo.roughnessFactor = 0.f;
                materialInfo.normalScale = 1.f;
                materialInfo.useColorTex = false;
                materialInfo.useEmissiveTex = false;
                materialInfo.useMetallicRoughnessTex = false;
                materialInfo.useNormalTex = false;
                materialInfo.useOcclusionTex = false;

            }

            float uvScale = staticMeshComponent.mTiling;

            if(staticMeshComponent.mTilesWithMeshScale)
            {
                float meshScale = transformComponent.GetWorldScaleUniform();
                float scaleFactor = meshScale < 1 && meshScale >0 ? 1 / meshScale : meshScale;
                uvScale *= scaleFactor;
            }

            materialInfo.uvScale = glm::vec4(uvScale, uvScale, 1.f, 1.f);

            mConstBuffers[InfoStruct::MATERIAL_INFO_CB]->Update(&materialInfo, sizeof(InfoStruct::DXMaterialInfo), meshCounter, frameIndex);
            meshCounter++;
        }
    }

    {
        // We get transform to make sure the mesh count is correct, since the user can have a mesh without a transform
        const auto view = mWorld.get().GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();

        for (auto [entity, skinnedMeshComponent, transformComponent] : view.each())
        {
            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                continue;
            }

            // Update material
            InfoStruct::DXMaterialInfo materialInfo{};
            if (skinnedMeshComponent.mMaterial != nullptr) 
            {
                SendMaterialTexturesToGPU(*skinnedMeshComponent.mMaterial);

                materialInfo.colorFactor = { skinnedMeshComponent.mMaterial->mBaseColorFactor.r,
                    skinnedMeshComponent.mMaterial->mBaseColorFactor.g,
                    skinnedMeshComponent.mMaterial->mBaseColorFactor.b,
                    0.f };

                materialInfo.emissiveFactor = { skinnedMeshComponent.mMaterial->mEmissiveFactor.r,
                    skinnedMeshComponent.mMaterial->mEmissiveFactor.g,
                    skinnedMeshComponent.mMaterial->mEmissiveFactor.b,
                    0.f };

                materialInfo.metallicFactor = skinnedMeshComponent.mMaterial->mMetallicFactor;
                materialInfo.roughnessFactor = skinnedMeshComponent.mMaterial->mRoughnessFactor;
                materialInfo.normalScale = skinnedMeshComponent.mMaterial->mNormalScale;

                materialInfo.useColorTex = skinnedMeshComponent.mMaterial->mBaseColorTexture != nullptr && skinnedMeshComponent.mMaterial->mBaseColorTexture->WasSentToGpu();
                materialInfo.useEmissiveTex = skinnedMeshComponent.mMaterial->mEmissiveTexture != nullptr && skinnedMeshComponent.mMaterial->mEmissiveTexture->WasSentToGpu();
                materialInfo.useMetallicRoughnessTex = skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr && skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture->WasSentToGpu();
                materialInfo.useNormalTex = skinnedMeshComponent.mMaterial->mNormalTexture != nullptr && skinnedMeshComponent.mMaterial->mNormalTexture->WasSentToGpu();
                materialInfo.useOcclusionTex = skinnedMeshComponent.mMaterial->mOcclusionTexture != nullptr && skinnedMeshComponent.mMaterial->mOcclusionTexture->WasSentToGpu();
            }
            else 
            {
                materialInfo.colorFactor = { 0.f, 0.f, 0.f, 0.f };
                materialInfo.emissiveFactor = { 0.f, 0.f, 0.f, 0.f };
                materialInfo.metallicFactor = 0.f;
                materialInfo.roughnessFactor = 0.f;
                materialInfo.normalScale = 1.f;
                materialInfo.useColorTex = false;
                materialInfo.useEmissiveTex = false;
                materialInfo.useMetallicRoughnessTex = false;
                materialInfo.useNormalTex = false;
                materialInfo.useOcclusionTex = false;
            }

            mConstBuffers[InfoStruct::MATERIAL_INFO_CB]->Update(&materialInfo, sizeof(InfoStruct::DXMaterialInfo), meshCounter, frameIndex);
            meshCounter++;
        }
    }

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

void CE::GPUWorld::SendMaterialTexturesToGPU(const CE::Material& mat)
{
    if (mat.mBaseColorTexture != nullptr
        && mat.mBaseColorTexture->IsReadyToBeSentToGpu())
    {
        mat.mBaseColorTexture->SendToGPU();
    }

    if (mat.mEmissiveTexture != nullptr
        && mat.mEmissiveTexture->IsReadyToBeSentToGpu())
    {
        mat.mEmissiveTexture->SendToGPU();
    }

    if (mat.mMetallicRoughnessTexture != nullptr
        && mat.mMetallicRoughnessTexture->IsReadyToBeSentToGpu())
    {
        mat.mMetallicRoughnessTexture->SendToGPU();
    }

    if (mat.mNormalTexture != nullptr
        && mat.mNormalTexture->IsReadyToBeSentToGpu())
    {
        mat.mNormalTexture->SendToGPU();
    }

    if (mat.mOcclusionTexture != nullptr
        && mat.mOcclusionTexture->IsReadyToBeSentToGpu())
    {
        mat.mOcclusionTexture->SendToGPU();
    }
}

void CE::GPUWorld::UpdateClusterData(const CameraComponent& camera)
{
    Device& engineDevice = Device::Get();
    int frameIndex = engineDevice.GetFrameIndex();

    InfoStruct::Clustering::DXCluster clusterInfo;
    clusterInfo.mNumClustersX = mClusterGrid.x;
    clusterInfo.mNumClustersY = mClusterGrid.y;
    clusterInfo.mNumClustersZ = mClusterGrid.z;
    mNumberOfClusters = mClusterGrid.x * mClusterGrid.y * mClusterGrid.z;
    clusterInfo.mMaxLightsInCluster = 50;
    mConstBuffers[InfoStruct::CLUSTER_INFO_CB]->Update(&clusterInfo, sizeof(InfoStruct::Clustering::DXCluster), 0, frameIndex);

    InfoStruct::Clustering::DXCameraClustering clusteringCam;
    clusteringCam.mFarPlane = camera.mFar;
    clusteringCam.mNearPlane = camera.mNear;
    clusteringCam.mDepthSliceScale = (float)clusterInfo.mNumClustersZ / std::log2f(clusteringCam.mFarPlane / clusteringCam.mNearPlane);
    clusteringCam.mDepthSliceBias = -((float)clusterInfo.mNumClustersZ * std::log2f(clusteringCam.mNearPlane) / std::log2f(clusteringCam.mFarPlane / clusteringCam.mNearPlane));
    clusteringCam.mLinearDepthCoefficient.x = clusteringCam.mFarPlane / (clusteringCam.mNearPlane - clusteringCam.mFarPlane);
    clusteringCam.mLinearDepthCoefficient.y = (clusteringCam.mNearPlane * clusteringCam.mFarPlane) / (clusteringCam.mNearPlane - clusteringCam.mFarPlane);
    clusteringCam.mScreenDimensions = camera.mViewportSize;
    clusteringCam.mTileSize = glm::vec2(clusteringCam.mScreenDimensions.x / clusterInfo.mNumClustersX, clusteringCam.mScreenDimensions.y / clusterInfo.mNumClustersY);
    mConstBuffers[InfoStruct::CLUSTERING_CAM_CB]->Update(&clusteringCam, sizeof(InfoStruct::Clustering::DXCameraClustering), 0, frameIndex);
}

void CE::GPUWorld::ClearClusterData()
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

    std::vector<uint32> clusterCompactData(4000, 0);
    D3D12_SUBRESOURCE_DATA data;
    data.pData = clusterCompactData.data();
    data.RowPitch = sizeof(uint32);
    data.SlicePitch = sizeof(uint32) * 4000;
    mStructuredBuffers[InfoStruct::COMPACT_CLUSTER_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    std::vector<int32> lightIndicesClear(4000, -1);
    data.pData = lightIndicesClear.data();
    data.RowPitch = sizeof(int32);
    data.SlicePitch = sizeof(int32) * 4000;
    mStructuredBuffers[InfoStruct::LIGHT_INDICES]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    uint32 counterValue = 0;
    data.pData = &counterValue;
    data.RowPitch = sizeof(uint32);
    data.SlicePitch = sizeof(uint32);
    mStructuredBuffers[InfoStruct::POINT_LIGHT_COUNTER]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);
    mStructuredBuffers[InfoStruct::CLUSTER_COUNTER_BUFFER]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    std::vector<InfoStruct::Clustering::DXLightGridElement> lightGridClear(4000, InfoStruct::Clustering::DXLightGridElement{});
    data.pData = lightGridClear.data();
    data.RowPitch = sizeof(InfoStruct::Clustering::DXLightGridElement);
    data.SlicePitch = sizeof(InfoStruct::Clustering::DXLightGridElement) * 4000;
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