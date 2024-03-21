#include "Precomp.h"
#include "Platform/PC/Rendering/RendererPC.h"
#include "Core/Device.h"
#include "Core/FileIO.h"

#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

#include "World/WorldRenderer.h"
#include "Components/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Utilities/DebugRenderer.h"

Engine::Renderer::Renderer()
{
    if (Device::IsHeadless())
    {
        return;
    }

    Device& engineDevice = Device::Get();

    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    //CREATE PBR PIPELINE
    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0");

    mPipelines[PBR_PIPELINE] = std::make_unique<DXPipeline>();
    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

    mPipelines[PBR_PIPELINE] = std::make_unique<DXPipeline>();
    mPipelines[PBR_PIPELINE]->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mPipelines[PBR_PIPELINE]->AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1);
    mPipelines[PBR_PIPELINE]->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2);
    mPipelines[PBR_PIPELINE]->AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3);
    mPipelines[PBR_PIPELINE]->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mPipelines[PBR_PIPELINE]->SetDepthState(depth);
    mPipelines[PBR_PIPELINE]->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPipelines[PBR_PIPELINE]->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"PBR RENDER PIPELINE");
    
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/ClusterGridCS.hlsl");
    ComPtr<ID3DBlob> cs = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0", true);
    mPipelines[CLUSTER_GRID_PIPELINE] = std::make_unique<DXPipeline>();
    mPipelines[CLUSTER_GRID_PIPELINE]->SetComputeShader(cs->GetBufferPointer(), cs->GetBufferSize());
    mPipelines[CLUSTER_GRID_PIPELINE]->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetComputeSignature()), L"CLUSTER GRID COMPUTE SHADER");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/CompactClustersCS.hlsl");
    cs = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0", true);
    mPipelines[COMPACT_CLUSTER_PIPELINE] = std::make_unique<DXPipeline>();
    mPipelines[COMPACT_CLUSTER_PIPELINE]->SetComputeShader(cs->GetBufferPointer(), cs->GetBufferSize());
    mPipelines[COMPACT_CLUSTER_PIPELINE]->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetComputeSignature()), L"COMPACT CLUSTER COMPUTE SHADER");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/CullClustersVS.hlsl");
    v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/CullClustersPS.hlsl");
    p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0");
    mPipelines[CULL_CLUSTER_PIPELINE] = std::make_unique<DXPipeline>();
    mPipelines[CULL_CLUSTER_PIPELINE]->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mPipelines[CULL_CLUSTER_PIPELINE]->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPipelines[CULL_CLUSTER_PIPELINE]->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetComputeSignature()), L"ACTIVE CLUSTER PIPELINE");

    //CREATE CONSTANT BUFFERS
    mConstBuffers[CAM_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 1, "Matrix buffer default shader", FRAME_BUFFER_COUNT);
    mConstBuffers[LIGHT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXLightInfo), 1, "Light info buffer", FRAME_BUFFER_COUNT);
    mConstBuffers[MODEL_INDEX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(int), MAX_MESHES + 2, "Model index", FRAME_BUFFER_COUNT);
    mConstBuffers[MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "Mesh matrix data", FRAME_BUFFER_COUNT);
    mConstBuffers[CLUSTER_INFO_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::Clustering::DXCluster), 1, "Cluster creation data", FRAME_BUFFER_COUNT);
    mConstBuffers[CLUSTERING_CAM_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::Clustering::DXCameraClustering), 1, "Clustering camera data", FRAME_BUFFER_COUNT);
   
    //CREATE STRUCTURED BUFFERS
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXMaterialInfo) * (MAX_MESHES + 2), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[MATERIAL_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "MATERIAL STRUCTURED BUFFER");
    mStructuredBuffers[MATERIAL_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXMaterialInfo)* (MAX_MESHES + 2), 0);
    
    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::Clustering::DXAABB) * 4000, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[CLUSTER_GRID_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "CLUSTER GRID BUFFER");
    
    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32) * 4000, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[ACTIVE_CLUSTER_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "ACTIVE CLUSTER BUFFER");
    mStructuredBuffers[ACTIVE_CLUSTER_SB]->CreateUploadBuffer(device, sizeof(uint32) * 4000, 0);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int) * 4000, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[COMPACT_CLUSTER_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "COMPACT CLUSTER BUFFER");
    mStructuredBuffers[COMPACT_CLUSTER_SB]->CreateUploadBuffer(device, sizeof(unsigned int) * 4000, 0);
    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[CLUSTER_COUNTER_BUFFER] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "COMPACT CLUSTER COUNTER BUFFER");

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXPointLightInfo) * 100, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[POINT_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "POINT LIGHT STRUCTURED BUFFER");
    mStructuredBuffers[POINT_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXPointLightInfo) * 100, 0);
    mPointLights.resize(100);

    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXDirLightInfo) * 100, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mStructuredBuffers[DIRECTIONAL_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "DIRECTIONAL LIGHT STRUCTURED BUFFER");
    mStructuredBuffers[DIRECTIONAL_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXDirLightInfo) * 100, 0);
    mDirectionalLights.resize(100);

    //CREATE SRVS
    //Materials
    D3D12_SHADER_RESOURCE_VIEW_DESC  srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXMaterialInfo);
    srvDesc.Buffer.NumElements = MAX_MESHES +2;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    materialHeapSlot = engineDevice.AllocateTexture(mStructuredBuffers[MATERIAL_SB].get(), srvDesc);
    mMaterialVec = std::vector<InfoStruct::DXMaterialInfo>(MAX_MESHES + 2);

    //Point lights
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXPointLightInfo);
    srvDesc.Buffer.NumElements = 100;
    mPointLightSRVIndex = engineDevice.AllocateTexture(mStructuredBuffers[POINT_LIGHT_SB].get(), srvDesc);

    //Directional lights
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXDirLightInfo);
    mDirectionalLightsSRVIndex = engineDevice.AllocateTexture(mStructuredBuffers[DIRECTIONAL_LIGHT_SB].get(), srvDesc);

    //AABB Clusters
    srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXAABB);
    srvDesc.Buffer.NumElements = 4000;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    mClusterSRVIndex = engineDevice.AllocateTexture(mStructuredBuffers[CLUSTER_GRID_SB].get(), srvDesc);

    //Active clusters
    srvDesc.Buffer.StructureByteStride = sizeof(uint32);
    mActiveClusterSRVIndex = engineDevice.AllocateTexture(mStructuredBuffers[ACTIVE_CLUSTER_SB].get(), srvDesc); 

    //Compact clusters
    mCompactClusterSRVIndex = engineDevice.AllocateTexture(mStructuredBuffers[COMPACT_CLUSTER_SB].get(), srvDesc); 

    //CREATE UAVS
    //AABB Clusters
    D3D12_UNORDERED_ACCESS_VIEW_DESC  uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.StructureByteStride = sizeof(InfoStruct::Clustering::DXAABB);
    uavDesc.Buffer.NumElements = 4000;
    mClusterUAVIndex = engineDevice.AllocateUAV(mStructuredBuffers[CLUSTER_GRID_SB].get(), uavDesc);

    //Active clusters
    uavDesc.Buffer.StructureByteStride = sizeof(uint32);
    mActiveClusterUAVIndex = engineDevice.AllocateUAV(mStructuredBuffers[ACTIVE_CLUSTER_SB].get(), uavDesc); 

    //Compact clusters
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    mCompactClusterUAVIndex = engineDevice.AllocateUAV(mStructuredBuffers[COMPACT_CLUSTER_SB].get(), uavDesc, mStructuredBuffers[CLUSTER_COUNTER_BUFFER].get()); 
}

void Engine::Renderer::Render(const World& world)
{
    // I'm not sure why, because I (Guus), know nothing of dx12, but dx12 does not like it
    // if we are sending textures to the GPU in the RENDERING pass. Soo my solution was to
    // do it here instead. The only downside is that we have to iterate over the static meshes
    // twice, but in entt this is surprisingly fast.
    { 
        const auto view = world.GetRegistry().View<const StaticMeshComponent>();

        for (auto [entity, staticMeshComponent] : view.each())
        {
            // It won't be rendered anyway,
            // so let's not bother finalising
            // the loading process.
            if (staticMeshComponent.mMaterial == nullptr
                || staticMeshComponent.mStaticMesh == nullptr)
            {
                continue;
            }

            const Material& mat = *staticMeshComponent.mMaterial;

            if (mat.mBaseColorTexture != nullptr)
            {
                (void)mat.mBaseColorTexture->GetIndex().has_value();
            }
			if (mat.mEmissiveTexture != nullptr)
			{
                (void)mat.mEmissiveTexture->GetIndex().has_value();
			}
			if (mat.mMetallicRoughnessTexture != nullptr)
			{
                (void)mat.mMetallicRoughnessTexture->GetIndex().has_value();
			}
			if (mat.mNormalTexture != nullptr)
			{
                (void)mat.mNormalTexture->GetIndex().has_value();
			}
			if (mat.mOcclusionTexture != nullptr)
			{
                (void)mat.mOcclusionTexture->GetIndex().has_value();
			}
        }
    }

    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
    int frameIndex = engineDevice.GetFrameIndex();

    
    //GET WORLD
    const auto optionalEntityCameraPair = world.GetRenderer().GetMainCamera();
    ASSERT_LOG(optionalEntityCameraPair.has_value(), "DX12 draw requests have been made, but they cannot be cleared as there is no camera to draw them to");

    //UPDATE CAMERA
    const auto camera = optionalEntityCameraPair->second;
    InfoStruct::DXMatrixInfo matrixInfo;
    matrixInfo.pm = glm::transpose(camera.GetProjection());
    matrixInfo.vm = glm::transpose(camera.GetView());

    matrixInfo.ipm = glm::inverse(matrixInfo.pm);
    matrixInfo.ivm = glm::inverse(matrixInfo.vm);
    mConstBuffers[CAM_MATRIX_CB]->Update(&matrixInfo, sizeof(InfoStruct::DXMatrixInfo), 0, frameIndex);

    if (glm::vec2(ImGui::GetContentRegionAvail()) != screenSize) {
        screenSize = ImGui::GetContentRegionAvail();
        updateClusterGrid = true;
    }

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    //UPDATE LIGHTS
    const auto pointLightView = world.GetRegistry().View<const PointLightComponent, const TransformComponent>();
    const auto dirLightView = world.GetRegistry().View<const DirectionalLightComponent, const TransformComponent>();
    int pointLightCounter = 0;
    int dirLightCounter = 0;

    for (auto [entity, lightComponent, transform] : pointLightView.each()) {
        if(pointLightCounter >= mPointLights.size())
        {
            mPointLights.resize(mPointLights.size() + 100);
            mStructuredBuffers[POINT_LIGHT_SB]->mResizeBuffer = true;
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
            mStructuredBuffers[DIRECTIONAL_LIGHT_SB]->mResizeBuffer = true;
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

    //USING CLUSTER CULLING AS A Z PRE PASS AS WELL
    CullClusters(world);

    commandList->SetGraphicsRootSignature(reinterpret_cast<DXSignature*>(engineDevice.GetSignature())->GetSignature().Get());
    commandList->SetPipelineState(mPipelines[PBR_PIPELINE]->GetPipeline().Get());

    //BIND CONSTANT BUFFERS
    mConstBuffers[LIGHT_CB]->Bind(commandList, 1, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 0, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 4, 0, frameIndex);
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 16, mDirectionalLightsSRVIndex);
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 17, mPointLightSRVIndex);

    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
    int meshCounter = 0;

    meshCounter = 0;
    //RENDERING
    for (auto [entity, staticMeshComponent, transform] : view.each())
    {
        if (!staticMeshComponent.mStaticMesh)
        {
            continue;
        }

        InfoStruct::DXMaterialInfo materialInfo;
        if (staticMeshComponent.mMaterial != nullptr) {
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
            materialInfo.useColorTex = staticMeshComponent.mMaterial->mBaseColorTexture != nullptr && staticMeshComponent.mMaterial->mBaseColorTexture->GetIndex().has_value();
            materialInfo.useEmissiveTex = staticMeshComponent.mMaterial->mEmissiveTexture != nullptr && staticMeshComponent.mMaterial->mEmissiveTexture->GetIndex().has_value();
            materialInfo.useMetallicRoughnessTex = staticMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr && staticMeshComponent.mMaterial->mMetallicRoughnessTexture->GetIndex().has_value();
            materialInfo.useNormalTex = staticMeshComponent.mMaterial->mNormalTexture != nullptr && staticMeshComponent.mMaterial->mNormalTexture->GetIndex().has_value();
            materialInfo.useOcclusionTex = staticMeshComponent.mMaterial->mOcclusionTexture != nullptr && staticMeshComponent.mMaterial->mOcclusionTexture->GetIndex().has_value();

            //BIND TEXTURES
            if (materialInfo.useColorTex)
            {
                resourceHeap->BindToGraphics(commandList, 5, *staticMeshComponent.mMaterial->mBaseColorTexture->GetIndex());
            }
            if (materialInfo.useEmissiveTex)
            {
                resourceHeap->BindToGraphics(commandList, 6, *staticMeshComponent.mMaterial->mEmissiveTexture->GetIndex());
            }
            if (materialInfo.useMetallicRoughnessTex)
            {
                resourceHeap->BindToGraphics(commandList, 7, *staticMeshComponent.mMaterial->mMetallicRoughnessTexture->GetIndex());
            }
            if (materialInfo.useNormalTex)
            {
                resourceHeap->BindToGraphics(commandList, 8, *staticMeshComponent.mMaterial->mNormalTexture->GetIndex());
            }
            if (materialInfo.useOcclusionTex)
            {
                resourceHeap->BindToGraphics(commandList, 9, *staticMeshComponent.mMaterial->mOcclusionTexture->GetIndex());
            }
        }
        else {
            materialInfo.colorFactor = { 0.f, 0.f, 0.f, 0.f };
            materialInfo.emissiveFactor = { 0.f, 0.f, 0.f, 0.f };
            materialInfo.metallicFactor = 0.f;
            materialInfo.roughnessFactor = 0.f;
            materialInfo.normalScale = 0.f;
            materialInfo.useColorTex = false;
            materialInfo.useEmissiveTex = false;
            materialInfo.useMetallicRoughnessTex = false;
            materialInfo.useNormalTex = false;
            materialInfo.useOcclusionTex = false;

        }
        mMaterialVec[meshCounter] = materialInfo;

        mConstBuffers[MODEL_MATRIX_CB]->Bind(commandList, 2, meshCounter, frameIndex);

        //UPDATE AND BIND MATERIAL INFO
        mConstBuffers[MODEL_INDEX_CB]->Update(&meshCounter, sizeof(int), meshCounter, frameIndex);
        mConstBuffers[MODEL_INDEX_CB]->Bind(commandList, 3, meshCounter, frameIndex);

        engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 15, materialHeapSlot);

        //DRAW THE MESH
        staticMeshComponent.mStaticMesh->DrawMesh();
        meshCounter++;
    }

    D3D12_SUBRESOURCE_DATA data;
    data.pData = mMaterialVec.data();
    data.RowPitch = sizeof(InfoStruct::DXMaterialInfo);
    data.SlicePitch = sizeof(InfoStruct::DXMaterialInfo) * (MAX_MESHES + 2);
    mStructuredBuffers[DXStructuredBuffers::MATERIAL_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);
    memset(mMaterialVec.data(), 0, sizeof(InfoStruct::DXMaterialInfo) * mMaterialVec.size());
    memset(mDirectionalLights.data(), 0, sizeof(InfoStruct::DXDirLightInfo) * mDirectionalLights.size());
    memset(mPointLights.data(), 0, sizeof(InfoStruct::DXPointLightInfo) * mPointLights.size());

    if (updateClusterGrid)
    {
        CalculateClusterGrid(camera);
        engineDevice.WaitForFence();
        CompactClusters();
        engineDevice.WaitForFence();
        //updateClusterGrid = false;
        commandList->SetGraphicsRootSignature(reinterpret_cast<DXSignature*>(engineDevice.GetSignature())->GetSignature().Get());
    }
}

Engine::MetaType Engine::Renderer::Reflect()
{
    return MetaType{ MetaType::T<Renderer>{}, "Renderer", MetaType::Base<System>{} };
}

void Engine::Renderer::CalculateClusterGrid(const CameraComponent& camera)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int frameIndex = engineDevice.GetFrameIndex();

    InfoStruct::Clustering::DXCluster clusterInfo;
    clusterInfo.mNumClustersX = 16;
    clusterInfo.mNumClustersY = 8;
    clusterInfo.mNumClustersZ = 24;
    mNumberOfTiles = 16 * 8 * 24;
    clusterInfo.mMaxLightsInCluster = 50;
    mConstBuffers[CLUSTER_INFO_CB]->Update(&clusterInfo, sizeof(InfoStruct::Clustering::DXCluster), 0, frameIndex);

    InfoStruct::Clustering::DXCameraClustering clusteringCam;
    clusteringCam.mFarPlane = camera.mFar;
    clusteringCam.mNearPlane = camera.mNear;
    clusteringCam.mDepthSliceScale = (float)clusterInfo.mNumClustersZ / std::log2f(clusteringCam.mFarPlane / clusteringCam.mNearPlane);
    clusteringCam.mDepthSliceBias = -((float)clusterInfo.mNumClustersZ * std::log2f(clusteringCam.mNearPlane) / std::log2f(clusteringCam.mFarPlane / clusteringCam.mNearPlane));
    clusteringCam.mLinearDepthCoefficient.x = clusteringCam.mFarPlane / (clusteringCam.mNearPlane - clusteringCam.mFarPlane);
    clusteringCam.mLinearDepthCoefficient.y = (clusteringCam.mNearPlane * clusteringCam.mFarPlane) / (clusteringCam.mNearPlane - clusteringCam.mFarPlane);
    clusteringCam.mScreenDimensions = screenSize;
    clusteringCam.mTileSize =  glm::vec2(clusteringCam.mScreenDimensions.x / clusterInfo.mNumClustersX, clusteringCam.mScreenDimensions.y /  clusterInfo.mNumClustersY);
    mConstBuffers[CLUSTERING_CAM_CB]->Update(&clusteringCam, sizeof(InfoStruct::Clustering::DXCameraClustering), 0, frameIndex);
    
    commandList->SetComputeRootSignature(reinterpret_cast<DXSignature*>(engineDevice.GetComputeSignature())->GetSignature().Get());
    commandList->SetPipelineState(mPipelines[CLUSTER_GRID_PIPELINE]->GetPipeline().Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    mConstBuffers[CLUSTER_INFO_CB]->BindToCompute(commandList, 0, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->BindToCompute(commandList, 1, 0, frameIndex);
    mConstBuffers[CLUSTERING_CAM_CB]->BindToCompute(commandList, 2, 0, frameIndex);
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 6, mClusterUAVIndex);

    commandList->Dispatch(clusterInfo.mNumClustersX, clusterInfo.mNumClustersY, clusterInfo.mNumClustersZ);
}

void Engine::Renderer::CullClusters(const World& world)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int frameIndex = engineDevice.GetFrameIndex();

    std::vector<uint32> activeClusters = std::vector<uint32>(4000, 0);
    D3D12_SUBRESOURCE_DATA data;
    data.pData = activeClusters.data();
    data.RowPitch = sizeof(uint32);
    data.SlicePitch = sizeof(uint32) * 4000;
    mStructuredBuffers[DXStructuredBuffers::ACTIVE_CLUSTER_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    commandList->SetGraphicsRootSignature(reinterpret_cast<DXSignature*>(engineDevice.GetComputeSignature())->GetSignature().Get());
    commandList->SetPipelineState(mPipelines[CULL_CLUSTER_PIPELINE]->GetPipeline().Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 1, 0, frameIndex);
    mConstBuffers[CLUSTERING_CAM_CB]->Bind(commandList, 2, 0, frameIndex);
    
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 8, mClusterSRVIndex);
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 7, mActiveClusterUAVIndex);

    int meshCounter = 0;
    const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
    for (auto [entity, staticMeshComponent, transform] : view.each()) {
        glm::mat4x4 modelMatrices[2]{};
        modelMatrices[0] = glm::transpose(transform.GetWorldMatrix());
        modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
        mConstBuffers[MODEL_MATRIX_CB]->Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);
        mConstBuffers[MODEL_MATRIX_CB]->Bind(commandList, 4, meshCounter, frameIndex);

        if (!staticMeshComponent.mStaticMesh)
            continue;

        staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
        meshCounter++;
    }

}

void Engine::Renderer::CompactClusters()
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

    commandList->SetComputeRootSignature(reinterpret_cast<DXSignature*>(engineDevice.GetComputeSignature())->GetSignature().Get());
    commandList->SetPipelineState(mPipelines[COMPACT_CLUSTER_PIPELINE]->GetPipeline().Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 6, mCompactClusterUAVIndex);
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 8, mActiveClusterSRVIndex);

    commandList->Dispatch(mNumberOfTiles, 1, 1);
}

void Engine::Renderer::UpdateLights(int numDirLights, int numPointLights)
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
    mConstBuffers[LIGHT_CB]->Update(&mLightInfo, sizeof(InfoStruct::DXLightInfo), 0, frameIndex);

    if (mStructuredBuffers[DIRECTIONAL_LIGHT_SB]->mResizeBuffer)
    {
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXDirLightInfo) * static_cast<uint>(mDirectionalLights.size()), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        mStructuredBuffers[DIRECTIONAL_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "DIRECTIONAL LIGHT STRUCTURED BUFFER");
        mStructuredBuffers[DIRECTIONAL_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXDirLightInfo) * static_cast<uint>(mDirectionalLights.size()), 0);
        srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXDirLightInfo);
        srvDesc.Buffer.NumElements =static_cast<uint>(mDirectionalLights.size());
        mDirectionalLightsSRVIndex = engineDevice.AllocateTexture(mStructuredBuffers[DIRECTIONAL_LIGHT_SB].get(), srvDesc);
        mStructuredBuffers[DIRECTIONAL_LIGHT_SB]->mResizeBuffer = false;
    }

    if (mStructuredBuffers[POINT_LIGHT_SB]->mResizeBuffer) {
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(InfoStruct::DXPointLightInfo) * static_cast<uint>(mPointLights.size()), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        mStructuredBuffers[POINT_LIGHT_SB] = std::make_unique<DXResource>(device, heapProperties, resourceDesc, nullptr, "POINT LIGHT STRUCTURED BUFFER");
        mStructuredBuffers[POINT_LIGHT_SB]->CreateUploadBuffer(device, sizeof(InfoStruct::DXPointLightInfo) *static_cast<uint>(mPointLights.size()), 0);
        srvDesc.Buffer.StructureByteStride = sizeof(InfoStruct::DXPointLightInfo);
        srvDesc.Buffer.NumElements = static_cast<uint>(mPointLights.size());
        mPointLightSRVIndex = engineDevice.AllocateTexture(mStructuredBuffers[POINT_LIGHT_SB].get(), srvDesc);
        mStructuredBuffers[POINT_LIGHT_SB]->mResizeBuffer = false;
    }
    
    D3D12_SUBRESOURCE_DATA data;
    data.pData = mDirectionalLights.data();
    data.RowPitch = sizeof(InfoStruct::DXDirLightInfo);
    data.SlicePitch = sizeof(InfoStruct::DXDirLightInfo) * numDirLights;
    mStructuredBuffers[DIRECTIONAL_LIGHT_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    data.pData = mPointLights.data();
    data.RowPitch = sizeof(InfoStruct::DXPointLightInfo);
    data.SlicePitch = sizeof(InfoStruct::DXPointLightInfo) * numPointLights;
    mStructuredBuffers[POINT_LIGHT_SB]->Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);


}
