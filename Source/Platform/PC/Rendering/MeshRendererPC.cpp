#include "Precomp.h"
#include "Platform/PC/Rendering/MeshRendererPC.h"
#include "Core/Device.h"
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
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaReflect.h"

#include "Rendering/GPUWorld.h"
#include "Components/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"
#include "Assets/SkinnedMesh.h"

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

    // Bind constant buffers
    gpuWorld.GetLightBuffer().Bind(commandList, 1, 0, frameIndex);
    gpuWorld.GetCameraBuffer().Bind(commandList, 0, 0, frameIndex);
    gpuWorld.GetCameraBuffer().Bind(commandList, 4, 0, frameIndex);

    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    int meshCounter = 0;
    commandList->SetPipelineState(mZPipeline.Get());

    // Depth pre-pass
    {
        const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
        
        for (auto [entity, staticMeshComponent, transform] : view.each()) 
        {
            if (!staticMeshComponent.mStaticMesh)
            {
                continue;
            }

            glm::mat4x4 modelMatrices[2]{};
            modelMatrices[0] = glm::transpose(transform.GetWorldMatrix());
            modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
            gpuWorld.GetModelMatrixBuffer().Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);
            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

            staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
            meshCounter++;
        }
    }

    commandList->SetPipelineState(mZSkinnedPipeline.Get());

    {
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();

        for (auto [entity, skinnedMeshComponent, transform] : view.each()) 
        {
            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                continue;
            }

            glm::mat4x4 modelMatrices[2]{};
            modelMatrices[0] = glm::transpose(transform.GetWorldMatrix());
            modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
            gpuWorld.GetModelMatrixBuffer().Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);
            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

            const auto& boneMatrices = skinnedMeshComponent.mFinalBoneMatrices;
            gpuWorld.GetBoneMatrixBuffer().Update(&boneMatrices.at(0), boneMatrices.size() * sizeof(glm::mat4x4), meshCounter, frameIndex);
            gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 5, meshCounter, frameIndex);

            skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
            meshCounter++;
        }
    }

    commandList->SetPipelineState(mPBRPipeline.Get());
    meshCounter = 0;

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
                if (staticMeshComponent.mMaterial->mBaseColorTexture && staticMeshComponent.mMaterial->mBaseColorTexture->WasSentToGpu())
                {
                    staticMeshComponent.mMaterial->mBaseColorTexture->BindToGraphics(commandList, 6);
                }
                if (staticMeshComponent.mMaterial->mEmissiveTexture && staticMeshComponent.mMaterial->mEmissiveTexture->WasSentToGpu())
                {
                    staticMeshComponent.mMaterial->mEmissiveTexture->BindToGraphics(commandList, 7);
                }
                if (staticMeshComponent.mMaterial->mMetallicRoughnessTexture && staticMeshComponent.mMaterial->mMetallicRoughnessTexture->WasSentToGpu())
                {
                    staticMeshComponent.mMaterial->mMetallicRoughnessTexture->BindToGraphics(commandList, 8);
                }
                if (staticMeshComponent.mMaterial->mNormalTexture && staticMeshComponent.mMaterial->mNormalTexture->WasSentToGpu())
                {
                    staticMeshComponent.mMaterial->mNormalTexture->BindToGraphics(commandList, 9);
                }
                if (staticMeshComponent.mMaterial->mOcclusionTexture && staticMeshComponent.mMaterial->mOcclusionTexture->WasSentToGpu())
                {
                    staticMeshComponent.mMaterial->mOcclusionTexture->BindToGraphics(commandList, 10);
                }
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);
            gpuWorld.GetMaterialInfoBuffer().Bind(commandList, 3, meshCounter, frameIndex);

            HandleColorComponent(world, entity, meshCounter, frameIndex);

            staticMeshComponent.mStaticMesh->DrawMesh();
            meshCounter++;
        }
    }
    
    // Render skinned meshes
    commandList->SetPipelineState(mPBRSkinnedPipeline.Get());

    {
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();
        
        for (auto [entity, skinnedMeshComponent, transform] : view.each())
        {
            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                continue;
            }

            // Bind textures
            if (skinnedMeshComponent.mMaterial)
            {
                if (skinnedMeshComponent.mMaterial->mBaseColorTexture && skinnedMeshComponent.mMaterial->mBaseColorTexture->WasSentToGpu())
                {
                    skinnedMeshComponent.mMaterial->mBaseColorTexture->BindToGraphics(commandList, 6);
                }
                if (skinnedMeshComponent.mMaterial->mEmissiveTexture && skinnedMeshComponent.mMaterial->mEmissiveTexture->WasSentToGpu())
                {
                    skinnedMeshComponent.mMaterial->mEmissiveTexture->BindToGraphics(commandList, 7);
                }
                if (skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture && skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture->WasSentToGpu())
                {
                    skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture->BindToGraphics(commandList, 8);
                }
                if (skinnedMeshComponent.mMaterial->mNormalTexture && skinnedMeshComponent.mMaterial->mNormalTexture->WasSentToGpu())
                {
                    skinnedMeshComponent.mMaterial->mNormalTexture->BindToGraphics(commandList, 9);
                }
                if (skinnedMeshComponent.mMaterial->mOcclusionTexture && skinnedMeshComponent.mMaterial->mOcclusionTexture->WasSentToGpu())
                {
                    skinnedMeshComponent.mMaterial->mOcclusionTexture->BindToGraphics(commandList, 10);
                }
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);
            gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 5, meshCounter, frameIndex);
            gpuWorld.GetMaterialInfoBuffer().Bind(commandList, 3, meshCounter, frameIndex);

            HandleColorComponent(world, entity, meshCounter, frameIndex);

            skinnedMeshComponent.mSkinnedMesh->DrawMesh();

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

    meshColorBuffer.Bind(commandList, 17, meshCounter, frameIndex);
}