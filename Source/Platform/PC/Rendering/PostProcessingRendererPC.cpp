#include "Precomp.h"
#include "Core/FileIO.h"
#include "Core/Device.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Rendering/GPUWorld.h"

#include "Platform/PC/Rendering/PostProcessingRendererPC.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/OutlineComponent.h"
#include "Platform/PC/Rendering/FramebufferPC.h"
#include "Rendering/Renderer.h"

CE::PostProcessingRenderer::PostProcessingRenderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/OutlinesVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/OutlinesPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0");

    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    mOutlinePipeline = DXPipelineBuilder()
        .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
        .AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 1)
        .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
        .SetDepthState(depth)
        .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
        .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"UI RENDER PIPELINE");
}

CE::PostProcessingRenderer::~PostProcessingRenderer()
{
}

void CE::PostProcessingRenderer::Render(const World& world)
{  
    RenderOutline(world);
}

void CE::PostProcessingRenderer::RenderOutline(const World& world)
{
    const auto view = world.GetRegistry().View<const PostPrOutlineComponent>();
    if (view.empty())
        return;

    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    GPUWorld& gpuWorld = world.GetGPUWorld();
    PosProcRenderingData& postProcData = gpuWorld.GetPostProcData();
    int frameIndex = engineDevice.GetFrameIndex();

    commandList->SetPipelineState(mOutlinePipeline.Get());
    postProcData.mOutlineBuffer->BindToCompute(commandList, 0, 0, frameIndex);
    gpuWorld.GetSelectedMeshFrameBuffer().BindSRVDepthToCompute(8);
    
    commandList->IASetVertexBuffers(0, 1, &postProcData.mVertexBufferView);
    commandList->IASetVertexBuffers(1, 1, &postProcData.mTexCoordBufferView);
    commandList->IASetIndexBuffer(&postProcData.mIndexBufferView);
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
