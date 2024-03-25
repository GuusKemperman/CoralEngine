#include "Precomp.h"
#include "Platform/PC/Rendering/UIRendererPC.h"
#include "Core/FileIO.h"
#include "Core/Device.h"
#include "Meta/MetaType.h"
#include "World/Registry.h"
#include "World/World.h"
#include "World/WorldRenderer.h"
#include "Components/UI/UISpriteComponent.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Assets/Texture.h"

Engine::UIRenderer::UIRenderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetUploadCommandList());
    engineDevice.StartUploadCommands();

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

    std::vector<glm::vec3> positions =
    {
        glm::vec3(-0.5f, 0.5f, 0.0f),  // Top Left
        glm::vec3(0.5f, 0.5f, 0.0f),   // Top Right
        glm::vec3(-0.5f, -0.5f, 0.0f), // Bottom Left
        glm::vec3(0.5f, -0.5f, 0.0f),  // Bottom Right
    };
    std::vector<glm::vec2> uvs = 
    {
        glm::vec2(0.f),
        glm::vec2(1.f, 0.f),
        glm::vec2(0.f, 1.f),
        glm::vec2(1.f, 1.f)
    };
    std::vector<uint32> indices = { 0, 1, 2, 3, 2, 1 };
    int vBufferSize = sizeof(glm::vec3) * static_cast<int>(positions.size());
    int tBufferSize = sizeof(glm::vec2) * static_cast<int>(uvs.size());
    int iBufferSize = sizeof(uint32) * static_cast<int>(indices.size());

    mQuadVResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), nullptr, "UI Vertex resource buffer");
    mQuadUVResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(tBufferSize), nullptr, "UI UV resource buffer");
    mIndicesResource = std::make_unique<DXResource>(device, CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), nullptr, "UI indices resource buffer");

    mQuadVResource->CreateUploadBuffer(device, vBufferSize, 0);
    mQuadUVResource->CreateUploadBuffer(device, tBufferSize, 0);
    mIndicesResource->CreateUploadBuffer(device, iBufferSize, 0);

    D3D12_SUBRESOURCE_DATA vData = {};
    vData.pData = positions.data();
    vData.RowPitch = sizeof(float) * 3;
    vData.SlicePitch = vBufferSize;

    D3D12_SUBRESOURCE_DATA uData = {};
    uData.pData = uvs.data();
    uData.RowPitch = sizeof(float) * 2;
    uData.SlicePitch = tBufferSize;

    D3D12_SUBRESOURCE_DATA iData = {};
    iData.pData = indices.data();
    iData.RowPitch = iBufferSize;
    iData.SlicePitch = iBufferSize;

    mQuadVResource->Update(commandList, vData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    mQuadUVResource->Update(commandList, uData, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, 0, 1);
    mIndicesResource->Update(commandList, iData, D3D12_RESOURCE_STATE_INDEX_BUFFER, 0, 1);

    mVertexBufferView.BufferLocation = mQuadVResource->GetResource()->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = sizeof(float) * 3;
    mVertexBufferView.SizeInBytes = vBufferSize;

    mTexCoordBufferView.BufferLocation = mQuadUVResource->GetResource()->GetGPUVirtualAddress();
    mTexCoordBufferView.StrideInBytes = sizeof(float) * 2;
    mTexCoordBufferView.SizeInBytes = tBufferSize;

    mIndexBufferView.BufferLocation = mIndicesResource->GetResource()->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mIndexBufferView.SizeInBytes = iBufferSize;

    mModelMatBuffer = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4) * 2, MAX_MESHES, "UI Mesh matrix data", FRAME_BUFFER_COUNT);
    mColorBuffer = std::make_unique<DXConstBuffer>(device, sizeof(ColorInfo), MAX_MESHES, "UI Mesh color data", FRAME_BUFFER_COUNT);
    engineDevice.SubmitUploadCommands();

}

Engine::UIRenderer::~UIRenderer()
{}

void Engine::UIRenderer::Render(const World& world)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
    int frameIndex = engineDevice.GetFrameIndex();

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
        mModelMatBuffer->Update(&modelMat, sizeof(ModelMat), spriteCount, frameIndex);

        mModelMatBuffer->Bind(commandList, 0, spriteCount, frameIndex);

        ColorInfo colorInfo;
        colorInfo.mColor = sprite.mColor;
        if (sprite.mTexture)
        {
            colorInfo.mUseTexture = true;
            sprite.mTexture->BindToGraphics(commandList, 6);
        }
        else
            colorInfo.mUseTexture = false;

        mColorBuffer->Update(&colorInfo, sizeof(ColorInfo), spriteCount, frameIndex);
        mColorBuffer->Bind(commandList, 1, spriteCount, frameIndex);

        commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
        commandList->IASetVertexBuffers(1, 1, &mTexCoordBufferView);
        commandList->IASetIndexBuffer(&mIndexBufferView);
        commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
    }
}

Engine::MetaType Engine::UIRenderer::Reflect()
{
	return MetaType{ MetaType::T<UIRenderer>{}, "UIRendererSystem", MetaType::Base<System>{} };
}
