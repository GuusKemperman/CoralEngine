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

Engine::UIRenderer::~UIRenderer() = default;

void Engine::UIRenderer::Render(const World& world)
{
    const Registry& reg = world.GetRegistry();

    struct DrawRequest
    {
        entt::entity mEntity{};
        float mDepth{};
    };
    std::vector<DrawRequest> drawRequests{};

    for (const auto [entity, transform, sprite] : reg.View<const TransformComponent, const UISpriteComponent>().each())
    {
	    if (sprite.mTexture == nullptr)
	    {
            continue;
	    }

		if (sprite.mTexture->IsReadyToBeSentToGpu())
	    {
            sprite.mTexture->SendToGPU();
	    }

        if (sprite.mTexture->WasSentToGpu())
        {
            drawRequests.emplace_back(DrawRequest{ entity, transform.GetWorldPosition()[Axis::Forward] });
        }
    }

    std::sort(drawRequests.begin(), drawRequests.end(),
        [](const DrawRequest& lhs, const DrawRequest& rhs)
        {
            return lhs.mDepth > rhs.mDepth;
        });

    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
    int frameIndex = engineDevice.GetFrameIndex();
    GPUWorld& gpuWorld = world.GetGPUWorld();
    UIRenderingData& renderingData = gpuWorld.GetUIRenderingData();

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 


    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetPipelineState(mPipeline->GetPipeline().Get());
    commandList->SetGraphicsRootSignature(reinterpret_cast<DXSignature*>(engineDevice.GetSignature())->GetSignature().Get());

    for (int i = 0; i < drawRequests.size(); i++)
    {
        const entt::entity entity = drawRequests[i].mEntity;
        const TransformComponent& transform = reg.Get<const TransformComponent>(entity);
        const UISpriteComponent& sprite = reg.Get<const UISpriteComponent>(entity);

        ModelMat modelMat;
        modelMat.mModel = glm::transpose(transform.GetWorldMatrix());
        modelMat.mTransposed = glm::transpose(modelMat.mModel);
        renderingData.mModelMatBuffer->Update(&modelMat, sizeof(ModelMat), i, frameIndex);
        renderingData.mModelMatBuffer->Bind(commandList, 0, i, frameIndex);

        InfoStruct::ColorInfo colorInfo;
        colorInfo.mColor = sprite.mColor;
        if (sprite.mTexture != nullptr
            && sprite.mTexture->WasSentToGpu())
        {
            colorInfo.mUseTexture = true;
            sprite.mTexture->BindToGraphics(commandList, 6);
        }
        else
        {
            colorInfo.mUseTexture = false;
        }

        renderingData.mColorBuffer->Update(&colorInfo, sizeof(InfoStruct::ColorInfo), i, frameIndex);
        renderingData.mColorBuffer->Bind(commandList, 1, i, frameIndex);

        commandList->IASetVertexBuffers(0, 1, &renderingData.mVertexBufferView);
        commandList->IASetVertexBuffers(1, 1, &renderingData.mTexCoordBufferView);
        commandList->IASetIndexBuffer(&renderingData.mIndexBufferView);
        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
    }
}
