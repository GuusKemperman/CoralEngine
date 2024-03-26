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

Engine::MeshRenderer::MeshRenderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    //CREATE PBR PIPELINE
    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");

    mPBRPipeline = std::make_unique<DXPipeline>();
    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

    mPBRPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mPBRPipeline->AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1);
    mPBRPipeline->AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 2);
    mPBRPipeline->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 3);
    mPBRPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mPBRPipeline->SetDepthState(depth);
    mPBRPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPBRPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"PBR RENDER PIPELINE");

    //CREATE PBR SKINNED PIPELINE
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertexSkinned.hlsl");
    v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
    mPBRSkinnedPipeline = std::make_unique<DXPipeline>();
    mPBRSkinnedPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mPBRSkinnedPipeline->AddInput("NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, 1);
    mPBRSkinnedPipeline->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2);
    mPBRSkinnedPipeline->AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3);
    mPBRSkinnedPipeline->AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 4);
    mPBRSkinnedPipeline->AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 5);
    mPBRSkinnedPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mPBRSkinnedPipeline->SetDepthState(depth);
    mPBRSkinnedPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPBRSkinnedPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"PBR SKINNED RENDER PIPELINE");
    
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZVertex.hlsl");
    v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mZPipeline = std::make_unique<DXPipeline>();
    mZPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mZPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mZPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0);
    mZPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"DEPTH RENDER PIPELINE");

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZSkinnedVertex.hlsl");
    v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    mZSkinnedPipeline = std::make_unique<DXPipeline>();
    mZSkinnedPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mZSkinnedPipeline->AddInput("BONEIDS", DXGI_FORMAT_R32G32B32A32_SINT, 1);
    mZSkinnedPipeline->AddInput("BONEWEIGHTS", DXGI_FORMAT_R32G32B32A32_FLOAT, 2);
    mZSkinnedPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mZSkinnedPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), nullptr, 0);
    mZSkinnedPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"SKINNED DEPTH RENDER PIPELINE");
}

Engine::MeshRenderer::~MeshRenderer() = default;

void Engine::MeshRenderer::Render(const World& world)
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
    commandList->SetPipelineState(mZPipeline->GetPipeline().Get());

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

    commandList->SetPipelineState(mZSkinnedPipeline->GetPipeline().Get());

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

    commandList->SetPipelineState(mPBRPipeline->GetPipeline().Get());
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
            const InfoStruct::DXMaterialInfo& materialInfo = gpuWorld.GetMaterial(meshCounter);
            if (staticMeshComponent.mMaterial)
            {
                if (materialInfo.useColorTex)
                {
                    staticMeshComponent.mMaterial->mBaseColorTexture->BindToGraphics(commandList, 6);
                }
                if (materialInfo.useEmissiveTex)
                {
                    staticMeshComponent.mMaterial->mEmissiveTexture->BindToGraphics(commandList, 7);
                }
                if (materialInfo.useMetallicRoughnessTex)
                {
                    staticMeshComponent.mMaterial->mMetallicRoughnessTexture->BindToGraphics(commandList, 8);
                }
                if (materialInfo.useNormalTex)
                {
                    staticMeshComponent.mMaterial->mNormalTexture->BindToGraphics(commandList, 9);
                }
                if (materialInfo.useOcclusionTex)
                {
                    staticMeshComponent.mMaterial->mOcclusionTexture->BindToGraphics(commandList, 10);
                }
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

            // Update mesh index
            gpuWorld.GetModelIndexBuffer().Update(&meshCounter, sizeof(int), meshCounter, frameIndex);
            gpuWorld.GetModelIndexBuffer().Bind(commandList, 3, meshCounter, frameIndex);

            engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 16, gpuWorld.GetMaterialHeapSlot());

            HandleColorComponent(world, entity, meshCounter, frameIndex);

            staticMeshComponent.mStaticMesh->DrawMesh();
            meshCounter++;
        }
    }
    
    // Render skinned meshes
    commandList->SetPipelineState(mPBRSkinnedPipeline->GetPipeline().Get());

    {
        const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();
        
        for (auto [entity, skinnedMeshComponent, transform] : view.each())
        {
            if (!skinnedMeshComponent.mSkinnedMesh)
            {
                continue;
            }

            // Bind textures
            const InfoStruct::DXMaterialInfo& materialInfo = gpuWorld.GetMaterial(meshCounter);
            if (skinnedMeshComponent.mMaterial)
            {
                if (materialInfo.useColorTex)
                {
                    skinnedMeshComponent.mMaterial->mBaseColorTexture->BindToGraphics(commandList, 6);
                }
                if (materialInfo.useEmissiveTex)
                {
                    skinnedMeshComponent.mMaterial->mEmissiveTexture->BindToGraphics(commandList, 7);
                }
                if (materialInfo.useMetallicRoughnessTex)
                {
                    skinnedMeshComponent.mMaterial->mMetallicRoughnessTexture->BindToGraphics(commandList, 8);
                }
                if (materialInfo.useNormalTex)
                {
                    skinnedMeshComponent.mMaterial->mNormalTexture->BindToGraphics(commandList, 9);
                }
                if (materialInfo.useOcclusionTex)
                {
                    skinnedMeshComponent.mMaterial->mOcclusionTexture->BindToGraphics(commandList, 10);
                }
            }

            gpuWorld.GetModelMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);
            gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 5, meshCounter, frameIndex);

            // Update mesh index
            gpuWorld.GetModelIndexBuffer().Update(&meshCounter, sizeof(int), meshCounter, frameIndex);
            gpuWorld.GetModelIndexBuffer().Bind(commandList, 3, meshCounter, frameIndex);

            engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->BindToGraphics(commandList, 16, gpuWorld.GetMaterialHeapSlot());

            HandleColorComponent(world, entity, meshCounter, frameIndex);

            skinnedMeshComponent.mSkinnedMesh->DrawMesh();

            meshCounter++;
        }
    }

    gpuWorld.UpdateMaterials();
}

void Engine::MeshRenderer::HandleColorComponent(const World& world, const entt::entity& entity, int meshCounter, int frameIndex)
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