#include "Precomp.h"
#include "Platform/PC/Rendering/MeshRendererPC.h"
#include "Core/Device.h"
#include "Rendering/Renderer.h"
#include "Core/FileIO.h"

#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/TransformComponent.h"
#include "Components/MeshColorComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"

#include "World/Registry.h"
#include "World/World.h"

#include "Rendering/GPUWorld.h"
#include "Components/CameraComponent.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Assets/SkinnedMesh.h"
#include "Platform/PC/Rendering/InfoStruct.h"

#ifdef EDITOR
#include "Rendering/FrameBuffer.h"
#endif

CE::MeshRenderer::MeshRenderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    //CREATE PBR PIPELINE
    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

    mPBRPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1)
        .AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 2)
        .AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 3)
        .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetDepthState(depth)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"PBR RENDER PIPELINE");

    //CREATE PBR SKINNED PIPELINE
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertexSkinned.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
    mPBRSkinnedPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1)
        .AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2)
        .AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3)
        .AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 4)
        .AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 5)
        .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetDepthState(depth)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"PBR SKINNED RENDER PIPELINE");
    
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZVertex.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mZPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0)
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"DEPTH RENDER PIPELINE");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZSkinnedVertex.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mZSkinnedPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 1)
        .AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 2)
        .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0)
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"SKINNED DEPTH RENDER PIPELINE");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/ClusterGridCS.hlsl");
    ComPtr<ID3DBlob> cs = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "cs_5_0");
    mClusterGridPipeline = DXPipelineBuilder()
        .SetComputeShader(cs->GetBufferPointer(), cs->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()), L"CLUSTER GRID COMPUTE SHADER");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/CompactClustersCS.hlsl");
    cs = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "cs_5_0");
    mCompactClusterPipeline = DXPipelineBuilder()
        .SetComputeShader(cs->GetBufferPointer(), cs->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()), L"COMPACT CLUSTER COMPUTE SHADER");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/AssignLightsCS.hlsl");
    cs = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "cs_5_0");
    mAssignLigthsPipeline = DXPipelineBuilder()
        .SetComputeShader(cs->GetBufferPointer(), cs->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()), L"ASSIGN LIGHTS COMPUTE SHADER");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/CullClustersVS.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/CullClustersPS.hlsl");
    p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0");
    mCullClusterPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .SetDepthState(depth)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()), L"CULL CLUSTER PIPELINE");
    
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/CullClustersSkinnedMeshVS.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mCullClusterSkinnedMeshPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 1)
        .AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 2)
        .SetDepthState(depth)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()), L"CULL CLUSTER PIPELINE");


    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZVertex.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    CD3DX12_RASTERIZER_DESC rast = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rast.DepthBias = 10000;
    rast.DepthBiasClamp = 0.0f;
    rast.SlopeScaledDepthBias = 1.0f;
    mShadowMapPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0)
        .SetRasterizer(rast)
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"SHADOWMAPPING PIPELINE");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZSkinnedVertex.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mShadowMapSkinnedPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 1)
        .AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 2)
        .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetRasterizer(rast)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0)
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"SKINNED SHADOW MAPPING PIPELINE");

}

CE::MeshRenderer::~MeshRenderer() = default;

void CE::MeshRenderer::Render(const World& world)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
    GPUWorld& gpuWorld = world.GetGPUWorld();
    int frameIndex = engineDevice.GetFrameIndex();

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    commandList->SetGraphicsRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()));

    RenderShadowMaps(world);

#ifdef EDITOR
    Renderer::Get().GetFrameBuffer().Bind();
#endif // EDITOR

    DepthPrePass(world, gpuWorld);
    ClusteredShading(world);

    commandList->SetGraphicsRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()));
    commandList->SetPipelineState(mPBRPipeline.Get());

    gpuWorld.GetCameraBuffer().Bind(commandList, 0, 0, frameIndex);
    gpuWorld.GetLightBuffer().Bind(commandList, 3, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTERING_CAM_CB).Bind(commandList, 6, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTER_INFO_CB).Bind(commandList, 7, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::FOG_CB).Bind(commandList, 18, 0, frameIndex);

    resourceHeap->BindToGraphics(commandList, 13, gpuWorld.GetDirLightHeapSlot());
    resourceHeap->BindToGraphics(commandList, 14, gpuWorld.GetPointLigthHeapSlot());
    resourceHeap->BindToGraphics(commandList, 15, gpuWorld.GetLigthGridSRVSlot());
    resourceHeap->BindToGraphics(commandList, 16, gpuWorld.GetLightIndicesSRVSlot());
    auto shadowMap = gpuWorld.GetShadowMap();
    shadowMap->mDepthResource->ChangeState(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);
    resourceHeap->BindToGraphics(commandList, 17, shadowMap->mDepthSRVHandle);

    int meshCounter = 0;
    {
        const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
        //RENDERING
        for (auto [entity, staticMeshComponent, transform] : view.each())
        {
            if (!staticMeshComponent.mStaticMesh)
            {
                continue;
            }

            // Bind textures
            if (staticMeshComponent.mMaterial)
            {
                if (staticMeshComponent.mMaterial->mBaseColorTexture != nullptr)
                {
                    staticMeshComponent.mMaterial->mBaseColorTexture->BindToGraphics(commandList, 8);
                }
                if (staticMeshComponent.mMaterial->mEmissiveTexture != nullptr)
                {
                    staticMeshComponent.mMaterial->mEmissiveTexture->BindToGraphics(commandList, 9);
                }
                if (staticMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr)
                {
                    staticMeshComponent.mMaterial->mMetallicRoughnessTexture->BindToGraphics(commandList, 10);
                }
                if (staticMeshComponent.mMaterial->mNormalTexture != nullptr)
                {
                    staticMeshComponent.mMaterial->mNormalTexture->BindToGraphics(commandList, 11);
                }
                if (staticMeshComponent.mMaterial->mOcclusionTexture != nullptr)
                {
                    staticMeshComponent.mMaterial->mOcclusionTexture->BindToGraphics(commandList, 12);
                }
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);
            gpuWorld.GetMaterialInfoBuffer().Bind(commandList, 4, meshCounter, frameIndex);

            HandleColorComponent(world, entity, meshCounter, frameIndex);

            staticMeshComponent.mStaticMesh->DrawMesh();
            meshCounter++;
        }
    }
    
    // Render skinned meshes
    commandList->SetPipelineState(mPBRSkinnedPipeline.Get());

    {
        int skinnedMeshCounter = 0;
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();
        
        for (auto [entity, skinnedMeshComponent, transform] : view.each())
        {
            if (skinnedMeshCounter >= MAX_SKINNED_MESHES)
            {
                LOG(LogRendering, Warning, "Attempted to draw more skinned meshes: {} than maximum: {}", skinnedMeshCounter, MAX_SKINNED_MESHES);
                break;
            }

            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                continue;
            }

            // Bind textures
            if (skinnedMeshComponent.mMaterial)
            {
                if (skinnedMeshComponent.mMaterial->mBaseColorTexture != nullptr)
                {
                    skinnedMeshComponent.mMaterial->mBaseColorTexture->BindToGraphics(commandList, 8);
                }
                if (skinnedMeshComponent.mMaterial->mEmissiveTexture != nullptr)
                {
                    skinnedMeshComponent.mMaterial->mEmissiveTexture->BindToGraphics(commandList, 9);
                }
                if (skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr)
                {
                    skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture->BindToGraphics(commandList, 10);
                }
                if (skinnedMeshComponent.mMaterial->mNormalTexture != nullptr)
                {
                    skinnedMeshComponent.mMaterial->mNormalTexture->BindToGraphics(commandList, 11);
                }
                if (skinnedMeshComponent.mMaterial->mOcclusionTexture != nullptr)
                {
                    skinnedMeshComponent.mMaterial->mOcclusionTexture->BindToGraphics(commandList, 12);
                }
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);
            gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);
            gpuWorld.GetMaterialInfoBuffer().Bind(commandList, 4, meshCounter, frameIndex);

            HandleColorComponent(world, entity, meshCounter, frameIndex);

            skinnedMeshComponent.mSkinnedMesh->DrawMesh();

            skinnedMeshCounter++;
            meshCounter++;
        }
    }

    commandList->SetGraphicsRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()));
}

void CE::MeshRenderer::DepthPrePass(const World& world, const GPUWorld& gpuWorld)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int meshCounter = 0;
    int frameIndex = engineDevice.GetFrameIndex();

    commandList->SetPipelineState(mZPipeline.Get());
    gpuWorld.GetCameraBuffer().Bind(commandList, 0, 0, frameIndex);

    {
        const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();

        for (auto [entity, staticMeshComponent, transform] : view.each()) 
        {
            if (!staticMeshComponent.mStaticMesh)
            {
                continue;
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);

            staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
            meshCounter++;
        }
    }

    commandList->SetPipelineState(mZSkinnedPipeline.Get());

    {
        int skinnedMeshCounter = 0;
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();

        for (auto [entity, skinnedMeshComponent, transform] : view.each()) 
        {
            skinnedMeshCounter++;if (skinnedMeshCounter >= MAX_SKINNED_MESHES)
            {
                LOG(LogRendering, Warning, "Attempted to draw more skinned meshes: {} than maximum: {}", skinnedMeshCounter, MAX_SKINNED_MESHES);
                break;
            }

            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                continue;
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);

            gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

            skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
            skinnedMeshCounter++;
            meshCounter++;
        }
    }

}

void CE::MeshRenderer::HandleColorComponent(const World& world, const entt::entity& entity, int meshCounter, int frameIndex)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    DXConstBuffer& meshColorBuffer = world.GetGPUWorld().GetMeshColorBuffer();

    auto colorComponent = world.GetRegistry().TryGet<MeshColorComponent>(entity);
    if (colorComponent)
    {
        InfoStruct::DXColorMultiplierInfo info;
        info.colorAdd = glm::vec4(colorComponent->mColorAddition, 0.f);
        info.colorMult = glm::vec4(colorComponent->mColorMultiplier, 0.f);
        meshColorBuffer.Update(&info, sizeof(InfoStruct::DXColorMultiplierInfo), meshCounter, frameIndex);
    }
    else
    {
        InfoStruct::DXColorMultiplierInfo info;
        info.colorAdd = glm::vec4(0.f);
        info.colorMult = glm::vec4(1.f);
        meshColorBuffer.Update(&info, sizeof(InfoStruct::DXColorMultiplierInfo), meshCounter, frameIndex);
    }

    meshColorBuffer.Bind(commandList, 5, meshCounter, frameIndex);
}

void CE::MeshRenderer::CalculateClusterGrid(const GPUWorld& gpuWorld)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int frameIndex = engineDevice.GetFrameIndex();

    commandList->SetComputeRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()));
    commandList->SetPipelineState(mClusterGridPipeline.Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTER_INFO_CB).BindToCompute(commandList, 0, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CAM_MATRIX_CB).BindToCompute(commandList, 1, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTERING_CAM_CB).BindToCompute(commandList, 2, 0, frameIndex);

    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 6, gpuWorld.GetClusterUAVSlot());

    glm::ivec3 clusterGrid = gpuWorld.GetClusterGrid();
    commandList->Dispatch(clusterGrid.x, clusterGrid.y, clusterGrid.z);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::CLUSTER_GRID_SB).Get(); // The resource that you're synchronizing.
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier);
}

void CE::MeshRenderer::CullClusters(const World& world, const GPUWorld& gpuWorld)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int frameIndex = engineDevice.GetFrameIndex();

    std::vector<uint32> activeClusters = std::vector<uint32>(gpuWorld.GetNumberOfClusters(), 0);
    D3D12_SUBRESOURCE_DATA data;
    data.pData = activeClusters.data();
    data.RowPitch = sizeof(uint32);
    data.SlicePitch = sizeof(uint32) * gpuWorld.GetNumberOfClusters();
    gpuWorld.GetStructuredBuffer(InfoStruct::ACTIVE_CLUSTER_SB).Update(commandList, data, D3D12_RESOURCE_STATE_GENERIC_READ, 0, 1);

    commandList->SetGraphicsRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()));
    commandList->SetPipelineState(mCullClusterPipeline.Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    gpuWorld.GetConstantBuffer(InfoStruct::CAM_MATRIX_CB).Bind(commandList, 1, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTERING_CAM_CB).Bind(commandList, 2, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTER_INFO_CB).Bind(commandList, 0, 0, frameIndex);

    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 10, gpuWorld.GetClusterSRVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 7, gpuWorld.GetActiveClusterUAVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 13, gpuWorld.GetLigthGridSRVSlot());

    int meshCounter = 0;
    const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
    for (auto [entity, staticMeshComponent, transform] : view.each())
    {
        gpuWorld.GetModelMatrixBuffer().Bind(commandList, 4, meshCounter, frameIndex);

        if (!staticMeshComponent.mStaticMesh)
            continue;

        staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
        meshCounter++;
    }

    commandList->SetPipelineState(mCullClusterSkinnedMeshPipeline.Get());
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    int skinnedMeshCounter = 0;
    const auto skinnedView = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();
    for (auto [entity, skinnedMeshComponent, transform] : skinnedView.each())
    {
        if (skinnedMeshCounter >= MAX_SKINNED_MESHES)
        {
            LOG(LogRendering, Warning, "Attempted to draw more skinned meshes: {} than maximum: {}", skinnedMeshCounter, MAX_SKINNED_MESHES);
            break;
        }
        
        if (!skinnedMeshComponent.mSkinnedMesh)
            continue;

        gpuWorld.GetModelMatrixBuffer().Bind(commandList, 4, meshCounter, frameIndex);

        gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 5, meshCounter, frameIndex);


        skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
        skinnedMeshCounter++;
        meshCounter++;
    }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::ACTIVE_CLUSTER_SB).Get();
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier);
}

void CE::MeshRenderer::CompactClusters(const GPUWorld& gpuWorld)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

    commandList->SetComputeRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()));
    commandList->SetPipelineState(mCompactClusterPipeline.Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 6, gpuWorld.GetCompactClusterUAVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 10, gpuWorld.GetActiveClusterSRVSlot());

    commandList->Dispatch(gpuWorld.GetNumberOfClusters(), 1, 1);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::COMPACT_CLUSTER_SB).Get(); // The resource that you're synchronizing.
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier);
}

void CE::MeshRenderer::AssignLights(const GPUWorld& gpuWorld, int compactClusterCount)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int frameIndex = engineDevice.GetFrameIndex();

    commandList->SetComputeRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()));
    commandList->SetPipelineState(mAssignLigthsPipeline.Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    gpuWorld.GetConstantBuffer(InfoStruct::LIGHT_CB).BindToCompute(commandList, 3, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CAM_MATRIX_CB).BindToCompute(commandList, 1, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTER_INFO_CB).BindToCompute(commandList, 0, 0, frameIndex);

    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 10, gpuWorld.GetClusterSRVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 11, gpuWorld.GetCompactClusterSRVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 13, gpuWorld.GetPointLigthHeapSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 6, gpuWorld.GetPointLightCounterUAVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 7, gpuWorld.GetLigthGridUAVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToCompute(commandList, 8, gpuWorld.GetLightIndicesUAVSlot());
    commandList->Dispatch(compactClusterCount, 1, 1);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::POINT_LIGHT_COUNTER).Get(); // The resource that you're synchronizing.
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier);

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::LIGHT_GRID_SB).Get(); // The resource that you're synchronizing.
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier);

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::LIGHT_INDICES).Get(); // The resource that you're synchronizing.
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier);
}

void CE::MeshRenderer::ClusteredShading(const World& world)
{
    GPUWorld& gpuWorld = world.GetGPUWorld();

    gpuWorld.ClearClusterData();
    CalculateClusterGrid(gpuWorld);
    CullClusters(world, gpuWorld);
    CompactClusters(gpuWorld);
    AssignLights(gpuWorld, gpuWorld.GetNumberOfClusters());
}

void CE::MeshRenderer::RenderShadowMaps(const World& world)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    const auto dirLightView = world.GetRegistry().View<const DirectionalLightComponent, const TransformComponent>();
    int frameIndex = engineDevice.GetFrameIndex();
    GPUWorld& gpuWorld = world.GetGPUWorld();

    commandList->SetPipelineState(mShadowMapPipeline.Get());

    int lightCounter = 1;
    gpuWorld.GetCameraBuffer().Bind(commandList, 0, 1, frameIndex);

    for (auto [entity, lightComponent, transform] : dirLightView.each()) {   
        commandList->SetPipelineState(mShadowMapPipeline.Get());
        if (!lightComponent.mCastShadows)
        {
            lightCounter++;
            continue;
        }
        glm::vec4 clearColor = glm::vec4(0.f);
        auto shadowMap = gpuWorld.GetShadowMap();

        shadowMap->mRenderTarget->ChangeState(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
        shadowMap->mDepthResource->ChangeState(commandList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        engineDevice.GetDescriptorHeap(DEPTH_HEAP)->ClearDepthStencil(commandList, shadowMap->mDepthHandle);
        engineDevice.GetDescriptorHeap(RT_HEAP)->ClearRenderTarget(commandList, shadowMap->mRTHandle, &clearColor[0]);

        commandList->RSSetViewports(1, &shadowMap->mViewport);
        commandList->RSSetScissorRects(1, &shadowMap->mScissorRect);
        engineDevice.GetDescriptorHeap(RT_HEAP)->BindRenderTargets(commandList, &shadowMap->mRTHandle, shadowMap->mDepthHandle);

        int meshCounter = 0;

        {
            const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
            for (auto [entity2, staticMeshComponent, transform2] : view.each()) 
            {
                if (!staticMeshComponent.mStaticMesh)
                {
                    continue;
                }

                gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);

                staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
                meshCounter++;
            }
        }

        commandList->SetPipelineState(mShadowMapSkinnedPipeline.Get());
        {
            int skinnedMeshCounter = 0;
            const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();
            for (auto [entity2, skinnedMeshComponent, transform2] : view.each())
            {
                if (skinnedMeshCounter >= MAX_SKINNED_MESHES)
                {
                    LOG(LogRendering, Warning, "Attempted to draw more skinned meshes: {} than maximum: {}", skinnedMeshCounter, MAX_SKINNED_MESHES);
                    break;
                }

                if (!skinnedMeshComponent.mSkinnedMesh)
                {
                    continue;
                }

                gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);
                gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

                skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
                skinnedMeshCounter++;
                meshCounter++;
            }
        }
        lightCounter++;

        Device::Get().BindSwapchainRT();
        return;
    }

    Device::Get().BindSwapchainRT();
}
