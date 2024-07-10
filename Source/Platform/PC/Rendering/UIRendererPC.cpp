#include "Precomp.h"
#include "Platform/PC/Rendering/UIRendererPC.h"
#include "Core/FileIO.h"
#include "Core/Device.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Components/UI/UISpriteComponent.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/UI/UITextComponent.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"
#include "Assets/Texture.h"
#include "Assets/Font.h"
#include "Rendering/GPUWorld.h"
#include <memory>


CE::UIRenderer::UIRenderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/UIVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/UIPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0");

    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    depth.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

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

    mPipeline = DXPipelineBuilder()
    .AddInput("POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0)
    .AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 1)
    .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
    .SetDepthState(depth)
    .SetBlendState(blendDesc)
    .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
    .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"UI RENDER PIPELINE");
    
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/TextVertex.hlsl");
    v = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/TextPixel.hlsl");
    p = DXPipelineBuilder::ShaderToBlob(shaderPath.c_str(), "ps_5_0");
    mTextPipeline = DXPipelineBuilder()
    .AddInput("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT)
    .AddInput("COLOR", DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT)
    .AddInput("UV_COORDS", DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT)
    .AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM)
    .SetDepthState(depth)
    .SetBlendState(blendDesc)
    .SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize())
    .Build(device, reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()), L"UI RENDER PIPELINE");
}

CE::UIRenderer::~UIRenderer() = default;

void CE::UIRenderer::Render(const World& world)
{
    mDrawRequests.clear();
    ProcessDrawRequests(world);

    for (int i = 0; i < mDrawRequests.size(); i++)
    {
        if (!mDrawRequests[i].mIsText)
            RenderUI(world, mDrawRequests[i]);
        else
            RenderText(world, mDrawRequests[i]);
    }
}

void CE::UIRenderer::ProcessDrawRequests(const World& world)
{
    const Registry& reg = world.GetRegistry();

    {
        uint16 spriteCounter = 0;
        for (const auto [entity, transform, sprite] : reg.View<const TransformComponent, const UISpriteComponent>().each())
        {
            if (sprite.mTexture == nullptr)
            {
                continue;
            }

            mDrawRequests.emplace_back(DrawRequest{ entity, transform.GetWorldPosition()[Axis::Forward], spriteCounter, false});
            spriteCounter++;
        }
    }

    {
        uint16 textCounter = 0;
        for (const auto [entity, transform, textComp] : reg.View<const TransformComponent, const UITextComponent>().each())
        {
            auto font = textComp.mFont;
            auto text = textComp.mText;

            if (!font || text.size() == 0)
                continue;

            mDrawRequests.emplace_back(DrawRequest{ entity, transform.GetWorldPosition()[Axis::Forward], textCounter, true});
            textCounter++;
        }
    }

    std::sort(mDrawRequests.begin(), mDrawRequests.end(),
        [](const DrawRequest& lhs, const DrawRequest& rhs)
        {
            return lhs.mDepth > rhs.mDepth;
        });
}

void CE::UIRenderer::RenderUI(const World& world, const DrawRequest& drawRequest)
{
    const Registry& reg = world.GetRegistry();

    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
    int frameIndex = engineDevice.GetFrameIndex();
    GPUWorld& gpuWorld = world.GetGPUWorld();
    UIRenderingData& renderingData = gpuWorld.GetUIRenderingData();

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 
    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    commandList->SetPipelineState(mPipeline.Get());
    commandList->SetGraphicsRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()));
    gpuWorld.GetCameraBuffer().Bind(commandList, 0, 2, frameIndex);

    const TransformComponent& transform = reg.Get<const TransformComponent>(drawRequest.mEntity);
    const UISpriteComponent& sprite = reg.Get<const UISpriteComponent>(drawRequest.mEntity);

    ModelMat modelMat;
    modelMat.mModel = glm::transpose(transform.GetWorldMatrix());
    modelMat.mTransposed = glm::transpose(modelMat.mModel);
    renderingData.mModelMatBuffer->Update(&modelMat, sizeof(ModelMat), drawRequest.mQuadIndex, frameIndex);
    renderingData.mModelMatBuffer->Bind(commandList, 1, drawRequest.mQuadIndex, frameIndex);

    InfoStruct::ColorInfo colorInfo;
    colorInfo.mColor = sprite.mColor;
    if (sprite.mTexture != nullptr)
    {
        colorInfo.mUseTexture = true;
        sprite.mTexture->BindToGraphics(commandList, 8);
    }
    else
    {
        colorInfo.mUseTexture = false;
    }

    renderingData.mColorBuffer->Update(&colorInfo, sizeof(InfoStruct::ColorInfo), drawRequest.mQuadIndex, frameIndex);
    renderingData.mColorBuffer->Bind(commandList, 3, drawRequest.mQuadIndex, frameIndex);

    commandList->IASetVertexBuffers(0, 1, &renderingData.mVertexBufferView);
    commandList->IASetVertexBuffers(1, 1, &renderingData.mTexCoordBufferView);
    commandList->IASetIndexBuffer(&renderingData.mIndexBufferView);
    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

}

void CE::UIRenderer::RenderText(const World& world, const DrawRequest& drawRequest)
{
    const Registry& reg = world.GetRegistry();

    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    int frameIndex = engineDevice.GetFrameIndex();
    GPUWorld& gpuWorld = world.GetGPUWorld();
    UIRenderingData& renderingData = gpuWorld.GetUIRenderingData();

    commandList->SetPipelineState(mTextPipeline.Get());
    commandList->SetGraphicsRootSignature(reinterpret_cast<ID3D12RootSignature*>(engineDevice.GetSignature()));
    gpuWorld.GetCameraBuffer().Bind(commandList, 0, 2, frameIndex);

    const UITextComponent& textComp = reg.Get<const UITextComponent>(drawRequest.mEntity);

    auto font = textComp.mFont;
    auto text = textComp.mText;

    auto& element = renderingData.mFontInfos[drawRequest.mQuadIndex];

    font->GetTexture().BindToGraphics(commandList, 8);
    commandList->IASetVertexBuffers(0, 1, &element.mVertexResourceView);
    commandList->IASetIndexBuffer(&element.mIndexResourceView);
    commandList->DrawIndexedInstanced(static_cast<UINT>(element.mIndexCount), 1, 0, 0, 0);
}
