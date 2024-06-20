#include "Precomp.h"
#include "Platform/PC/Rendering/MeshRendererPC.h"
#include "Core/Device.h"
#include "Rendering/Renderer.h"
#include "Rendering/FrameBuffer.h"
#include "Core/FileIO.h"

#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Platform/PC/Rendering/FramebufferPC.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/TransformComponent.h"
#include "Components/MeshColorComponent.h"
#include "Components/DirectionalLightComponent.h"

#include "World/Registry.h"
#include "World/World.h"

#include "Rendering/GPUWorld.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Assets/SkinnedMesh.h"
#include "Platform/PC/Rendering/InfoStruct.h"
#include "Rendering/FrameBuffer.h"

CE::MeshRenderer::MeshRenderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    //CREATE PBR PIPELINE
    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/SimpleDiffusePixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    mPBRPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1)
        .AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 2)
        .AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 3)
        .AddRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthState(depth)
        .SetMsaaCountAndQuality(MSAA_COUNT, MSAA_QUALITY)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"PBR RENDER PIPELINE");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRParticlePixel.hlsl");
    p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");

    CD3DX12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
    depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    mParticlePBRPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1)
        .AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 2)
        .AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 3)
        .SetBlendState(blendDesc)
        .AddRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetMsaaCountAndQuality(MSAA_COUNT, MSAA_QUALITY)
        .SetDepthState(depth)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"PBR RENDER PIPELINE");

    //CREATE PBR SKINNED PIPELINE
    depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertexSkinned.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/SimpleDiffusePixel.hlsl");
    p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
    mPBRSkinnedPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1)
        .AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2)
        .AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3)
        .AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 4)
        .AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 5)
        .AddRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetDepthState(depth)
        .SetMsaaCountAndQuality(MSAA_COUNT, MSAA_QUALITY)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"PBR SKINNED RENDER PIPELINE");
    
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZVertex.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mZPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0)
        .SetMsaaCountAndQuality(MSAA_COUNT, MSAA_QUALITY)
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"DEPTH RENDER PIPELINE");

    mZSelectedPipeline = DXPipelineBuilder()
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
        .AddRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0)
        .SetMsaaCountAndQuality(MSAA_COUNT, MSAA_QUALITY)
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"SKINNED DEPTH RENDER PIPELINE");

    mZSelectedSkinnedPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 1)
        .AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 2)
        .AddRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT)
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
        .SetMsaaCountAndQuality(MSAA_COUNT, MSAA_QUALITY)
        .AddRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()), L"CULL CLUSTER PIPELINE");
    
    //Requires different depth settings because of particle transparency, therefore it needs a separate PSO
    depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    mCullClusterParticlePipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .SetDepthState(depth)
        .SetMsaaCountAndQuality(MSAA_COUNT, MSAA_QUALITY)
        .AddRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetComputeSignature()), L"CULL CLUSTER PIPELINE");

    depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/Clustering/CullClustersSkinnedMeshVS.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mCullClusterSkinnedMeshPipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 1)
        .AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 2)
        .SetDepthState(depth)
        .SetMsaaCountAndQuality(MSAA_COUNT, MSAA_QUALITY)
        .AddRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT)
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

    gpuWorld.GetMsaaFrameBuffer().Bind();
    gpuWorld.GetMsaaFrameBuffer().Clear();

    RenderShadowMaps(world);

    gpuWorld.GetMsaaFrameBuffer().Bind();

    DepthPrePass(world, gpuWorld);

    gpuWorld.GetMsaaFrameBuffer().Bind();

    ClusteredShading(world);

    commandList->SetGraphicsRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()));

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
    
    // Render skinned meshes
    commandList->SetPipelineState(mPBRSkinnedPipeline.Get());
    int meshCounter = 0;

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
                meshCounter++;
                continue;
            }

            if (skinnedMeshComponent.mMaterial)
            {
                BindMaterial(*skinnedMeshComponent.mMaterial);
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

    commandList->SetPipelineState(mPBRPipeline.Get());
    {
        const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
        for (auto [entity, staticMeshComponent, transform] : view.each())
        {
            if (!staticMeshComponent.mStaticMesh)
            {
                meshCounter++;
                continue;
            }

            if (staticMeshComponent.mMaterial)
            {
                BindMaterial(*staticMeshComponent.mMaterial);
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);
            gpuWorld.GetMaterialInfoBuffer().Bind(commandList, 4, meshCounter, frameIndex);

            HandleColorComponent(world, entity, meshCounter, frameIndex);

            staticMeshComponent.mStaticMesh->DrawMesh();
            meshCounter++;
        }
    }

    RenderParticles(world);

    commandList->SetGraphicsRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()));
}

void CE::MeshRenderer::DepthPrePass(const World& world, const GPUWorld& gpuWorld)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int meshCounter = 0;
    int frameIndex = engineDevice.GetFrameIndex();

    gpuWorld.GetCameraBuffer().Bind(commandList, 0, 0, frameIndex);
    commandList->SetPipelineState(mZSkinnedPipeline.Get());
    {
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();

        for (auto [entity, skinnedMeshComponent, transform] : view.each()) 
        {
            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                meshCounter++;
                continue;
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);
            gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

            skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
            meshCounter++;
        }
    }

    commandList->SetPipelineState(mZPipeline.Get());
    {
        const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();

        for (auto [entity, staticMeshComponent, transform] : view.each()) 
        {
            if (!staticMeshComponent.mStaticMesh)
            {
                meshCounter++;
                continue;
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);

            staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();

            meshCounter++;

        }
    }

    meshCounter = 0;
    gpuWorld.GetSelectionFramebuffer().Bind();
    gpuWorld.GetSelectionFramebuffer().Clear();

    commandList->SetPipelineState(mZSelectedSkinnedPipeline.Get());
    {
        int skinnedMeshCounter = 0;
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();

        for (auto [entity, skinnedMeshComponent, transform] : view.each()) 
        {
            if (!skinnedMeshComponent.mSkinnedMesh || !skinnedMeshComponent.mHighlightedMesh)
            {
                meshCounter++;
                continue;
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);
            gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

            skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
            skinnedMeshCounter++;
            meshCounter++;
        }
    }

    commandList->SetPipelineState(mZSelectedPipeline.Get());
    {
        const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();

        for (auto [entity, staticMeshComponent, transform] : view.each()) 
        {
            if (!staticMeshComponent.mStaticMesh || !staticMeshComponent.mHighlightedMesh)
            {
                meshCounter++;
                continue;
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);
            staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
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
    ID3D12DescriptorHeap* descriptorHeaps[] = {engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    gpuWorld.GetConstantBuffer(InfoStruct::CAM_MATRIX_CB).Bind(commandList, 1, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTERING_CAM_CB).Bind(commandList, 2, 0, frameIndex);
    gpuWorld.GetConstantBuffer(InfoStruct::CLUSTER_INFO_CB).Bind(commandList, 0, 0, frameIndex);

    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 10, gpuWorld.GetClusterSRVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 7, gpuWorld.GetActiveClusterUAVSlot());
    engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 13, gpuWorld.GetLigthGridSRVSlot());

    commandList->SetPipelineState(mCullClusterSkinnedMeshPipeline.Get());
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    int skinnedMeshCounter = 0;
    int meshCounter = 0;

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

        if (!skinnedMeshComponent.mSkinnedMesh)
        {
            meshCounter++;
            continue;
        }

        skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
        skinnedMeshCounter++;
        meshCounter++;
    }

    commandList->SetPipelineState(mCullClusterPipeline.Get());
    const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
    for (auto [entity, staticMeshComponent, transform] : view.each())
    {
        gpuWorld.GetModelMatrixBuffer().Bind(commandList, 4, meshCounter, frameIndex);

        if (!staticMeshComponent.mStaticMesh)
        {
            meshCounter++;
            continue;
        }

        staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
        meshCounter++;
    }

    commandList->SetPipelineState(mCullClusterParticlePipeline.Get());
    uint32 particleCount = gpuWorld.GetNumParticles();
    for (uint32 i = 0; i < particleCount; i++)
    {
        InfoStruct::DXParticleInfo particle = gpuWorld.GetParticle(i);
        gpuWorld.GetConstantBuffer(InfoStruct::PARTICLE_MODEL_MATRIX_CB).Bind(commandList, 4, i, frameIndex);
        particle.mMesh->DrawMeshVertexOnly();
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
    barrier.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::POINT_LIGHT_COUNTER).Get();
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier);

    D3D12_RESOURCE_BARRIER barrier2 = {};
    barrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier2.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::LIGHT_GRID_SB).Get();
    barrier2.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier2);

    D3D12_RESOURCE_BARRIER barrier3 = {};
    barrier3.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier3.UAV.pResource = gpuWorld.GetStructuredBuffer(InfoStruct::LIGHT_INDICES).Get();
    barrier3.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    commandList->ResourceBarrier(1, &barrier3);
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
                    meshCounter++;
                    continue;
                }

                gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);
                gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

                skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
                skinnedMeshCounter++;
                meshCounter++;
            }
        }

        commandList->SetPipelineState(mShadowMapPipeline.Get());
        {
            const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
            for (auto [entity2, staticMeshComponent, transform2] : view.each()) 
            {
                if (!staticMeshComponent.mStaticMesh)
                {
                    meshCounter++;
                    continue;
                }

                gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);

                staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
                meshCounter++;
            }
        }
        lightCounter++;

        return;
    }

}

void CE::MeshRenderer::RenderParticles(const World& world)
{
    Device& engineDevice = Device::Get();
    GPUWorld& gpuWorld = world.GetGPUWorld();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);

    int frameIndex = engineDevice.GetFrameIndex();
    uint32 particleCount = gpuWorld.GetNumParticles();

    commandList->SetPipelineState(mParticlePBRPipeline.Get());

    for (uint32 i = 0; i < particleCount; i++)
    {
        InfoStruct::DXParticleInfo particle = gpuWorld.GetParticle(i);

        if (particle.mMaterial)
        {
            BindMaterial(*particle.mMaterial);
        }


        gpuWorld.GetConstantBuffer(InfoStruct::PARTICLE_MODEL_MATRIX_CB).Bind(commandList, 1, i, frameIndex);
        gpuWorld.GetConstantBuffer(InfoStruct::PARTICLE_COLOR_CB).Bind(commandList, 5, i, frameIndex);
        gpuWorld.GetConstantBuffer(InfoStruct::PARTICLE_MATERIAL_INFO_CB).Bind(commandList, 4, i, frameIndex);
        gpuWorld.GetConstantBuffer(InfoStruct::PARTICLE_INFO_CB).Bind(commandList, 19, i, frameIndex);
        particle.mMesh->DrawMesh();
    }
}

void CE::MeshRenderer::BindMaterial(const CE::Material& material)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());

    if (material.mBaseColorTexture != nullptr)
    {
        material.mBaseColorTexture->BindToGraphics(commandList, 8);
    }
    if (material.mEmissiveTexture != nullptr)
    {
        material.mEmissiveTexture->BindToGraphics(commandList, 9);
    }
    if (material.mMetallicRoughnessTexture != nullptr)
    {
        material.mMetallicRoughnessTexture->BindToGraphics(commandList, 10);
    }
    if (material.mNormalTexture != nullptr)
    {
        material.mNormalTexture->BindToGraphics(commandList, 11);
    }
    if (material.mOcclusionTexture != nullptr)
    {
        material.mOcclusionTexture->BindToGraphics(commandList, 12);
    }
}
