#include "Precomp.h"
#include "Platform/PC/Rendering/ShadowMapRendererPC.h"
#include "Core/Device.h"
#include "Core/FileIO.h"

#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/TransformComponent.h"
#include "Components/MeshColorComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"

#include "Rendering/GPUWorld.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"
#include "Meta/MetaReflect.h"

#include "Assets/StaticMesh.h"
#include "Assets/SkinnedMesh.h"

CE::ShadowMapRenderer::ShadowMapRenderer()
{
    FileIO& fileIO = FileIO::Get();
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/ZVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
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

CE::ShadowMapRenderer::~ShadowMapRenderer() = default;

void CE::ShadowMapRenderer::Render(const World& world)
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

                glm::mat4x4 modelMatrices[2]{};
                modelMatrices[0] = glm::transpose(transform.GetWorldMatrix());
                modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
                gpuWorld.GetModelMatrixBuffer().Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);
                gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);

                staticMeshComponent.mStaticMesh->DrawMeshVertexOnly();
                meshCounter++;
            }
        }

        commandList->SetPipelineState(mShadowMapSkinnedPipeline.Get());
        {
            const auto view = world.GetRegistry().View<const SkinnedMeshComponent, const TransformComponent>();
            for (auto [entity2, skinnedMeshComponent, transform2] : view.each())
            {
                if (!skinnedMeshComponent.mSkinnedMesh)
                {
                    continue;
                }

                glm::mat4x4 modelMatrices[2]{};
                modelMatrices[0] = glm::transpose(transform.GetWorldMatrix());
                modelMatrices[1] = glm::transpose(glm::inverse(modelMatrices[0]));
                gpuWorld.GetModelMatrixBuffer().Update(&modelMatrices, sizeof(glm::mat4x4) * 2, meshCounter, frameIndex);
                gpuWorld.GetModelMatrixBuffer().Bind(commandList, 1, meshCounter, frameIndex);

                const auto& boneMatrices = skinnedMeshComponent.mFinalBoneMatrices;
                gpuWorld.GetBoneMatrixBuffer().Update(&boneMatrices.at(0), boneMatrices.size() * sizeof(glm::mat4x4), meshCounter, frameIndex);
                gpuWorld.GetBoneMatrixBuffer().Bind(commandList, 2, meshCounter, frameIndex);

                skinnedMeshComponent.mSkinnedMesh->DrawMeshVertexOnly();
                meshCounter++;
            }
        }
        lightCounter++;

        return;
    }

}
