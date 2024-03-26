#include "Precomp.h"
#include "Platform/PC/Rendering/UIRendererPC.h"
#include "Core/FileIO.h"
#include "Core/Device.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Components/UI/UISpriteComponent.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Assets/Texture.h"
#include "Rendering/GPUWorld.h"

Engine::UIRenderer::UIRenderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/UIVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/UIPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0");

    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    mPipeline = std::make_unique<DXPipeline>();
    mPipeline->AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0);
    mPipeline->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 1);
    mPipeline->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mPipeline->SetDepthState(depth);
    mPipeline->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPipeline->CreatePipeline(device, reinterpret_cast<DXSignature*>(engineDevice.GetSignature()), L"UI RENDER PIPELINE");
}

Engine::UIRenderer::~UIRenderer()
{}

void Engine::UIRenderer::Render(const World& world)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
    int frameIndex = engineDevice.GetFrameIndex();
    GPUWorld& gpuWorld = world.GetGPUWorld();

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 

    const auto spriteView = world.GetRegistry().View<const TransformComponent, const UISpriteComponent>();
    int spriteCount = 0;

    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetPipelineState(mPipeline->GetPipeline().Get());
    commandList->SetGraphicsRootSignature(reinterpret_cast<DXSignature*>(engineDevice.GetSignature())->GetSignature().Get());
    
    for (auto [entity, transform, sprite] : spriteView.each())
    {
        ModelMat modelMat;
        modelMat.mModel = glm::transpose(transform.GetWorldMatrix());
        modelMat.mTransposed = glm::transpose(modelMat.mModel);
        gpuWorld.GetUIRenderingData().mModelMatBuffer->Update(&modelMat, sizeof(ModelMat), spriteCount, frameIndex);
        gpuWorld.GetUIRenderingData().mModelMatBuffer->Bind(commandList, 0, spriteCount, frameIndex);

        InfoStruct::ColorInfo colorInfo;
        colorInfo.mColor = sprite.mColor;
        if (sprite.mTexture)
        {
            colorInfo.mUseTexture = true;
            sprite.mTexture->BindToGraphics(commandList, 6);
        }
        else
            colorInfo.mUseTexture = false;

        gpuWorld.GetUIRenderingData().mColorBuffer->Update(&colorInfo, sizeof(InfoStruct::ColorInfo), spriteCount, frameIndex);
        gpuWorld.GetUIRenderingData().mColorBuffer->Bind(commandList, 1, spriteCount, frameIndex);

        gpuWorld.GetUIRenderingData().RenderData(commandList);
    }
}
