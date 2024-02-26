#include "Precomp.h"
#include "Platform/PC/Rendering/RendererPC.h"
#include "Core/Device.h"

#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"

#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

//#include "Components/StaticMeshComponent.h"
//#include "Components/TransformComponent.h"
//#include "World/World.h"
//#include "World/Registry.h"

Engine::Renderer::Renderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());
    IDXGISwapChain3* swapchain = reinterpret_cast<IDXGISwapChain3*>(engineDevice.GetSwapchain());
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());


    //CREATE ROOT SIGNATURE
    mSignature = std::make_unique<DXSignature>(11);
    mSignature->AddCBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
    mSignature->AddCBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);
    mSignature->AddCBuffer(2, D3D12_SHADER_VISIBILITY_PIXEL);
    mSignature->AddCBuffer(3, D3D12_SHADER_VISIBILITY_PIXEL);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 1);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 2);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 3);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 4);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);
    mSignature->AddCBuffer(4, D3D12_SHADER_VISIBILITY_VERTEX);
    mSignature->CreateSignature(device, L"MAIN ROOT SIGNATURE");

    //CREATE PBR PIPELINE
    ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob("assets/shaders/PBRVertex.hlsl", "vs_5_0");
    ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob("assets/shaders/PBRPixel.hlsl", "ps_5_0", "main");
    mPipelines[PBR_PIPELINE] = std::make_unique<DXPipeline>();
    mPipelines[PBR_PIPELINE]->AddInput("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
    mPipelines[PBR_PIPELINE]->AddInput("NORMAL", DXGI_FORMAT_R32G32B32A32_FLOAT, 1);
    mPipelines[PBR_PIPELINE]->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2);
    mPipelines[PBR_PIPELINE]->AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3);
    mPipelines[PBR_PIPELINE]->AddRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM);
    mPipelines[PBR_PIPELINE]->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPipelines[PBR_PIPELINE]->CreatePipeline(device, mSignature, L"PBR RENDER PIPELINE");

    //CREATE SKY PIPELINE
    CD3DX12_RASTERIZER_DESC rast = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    CD3DX12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    rast.CullMode = D3D12_CULL_MODE_NONE;
    depth.DepthEnable = FALSE;
    depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    v = DXPipeline::ShaderToBlob("assets/shaders/SkyboxVertex.hlsl", "vs_5_0");
    p = DXPipeline::ShaderToBlob("assets/shaders/SkyboxPixel.hlsl", "ps_5_0", "main");
    mPipelines[SKY_PIPELINE] = std::make_unique<DXPipeline>();
    mPipelines[SKY_PIPELINE]->AddInput("POSITION", DXGI_FORMAT_R32G32B32A32_FLOAT, 0);
    mPipelines[SKY_PIPELINE]->AddInput("NORMAL", DXGI_FORMAT_R32G32B32A32_FLOAT, 1);
    mPipelines[SKY_PIPELINE]->AddInput("TEXCOORD", DXGI_FORMAT_R32G32_FLOAT, 2);
    mPipelines[SKY_PIPELINE]->AddInput("TANGENT", DXGI_FORMAT_R32G32B32_FLOAT, 3);
    mPipelines[SKY_PIPELINE]->SetVertexAndPixelShaders(v->GetBufferPointer(), v->GetBufferSize(), p->GetBufferPointer(), p->GetBufferSize());
    mPipelines[SKY_PIPELINE]->SetDepthState(depth);
    mPipelines[SKY_PIPELINE]->SetRasterizer(rast);
    mPipelines[SKY_PIPELINE]->CreatePipeline(device, mSignature, L"SKYBOX SIGNATURE");

    //CREATE CONSTANT BUFFERS
    mConstBuffers[CAM_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMatrixInfo), 1, "Matrix buffer default shader", FRAME_BUFFER_COUNT);
    mConstBuffers[LIGHT_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXLightInfo), 1, "Point light buffer", FRAME_BUFFER_COUNT);
    mConstBuffers[MATERIAL_CB] = std::make_unique<DXConstBuffer>(device, sizeof(InfoStruct::DXMaterialInfo), MAX_MESHES + 2, "Material info data", FRAME_BUFFER_COUNT);
    mConstBuffers[MESH_INDEX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(int), MAX_MESHES + 11000, "Mesh index data", FRAME_BUFFER_COUNT);

    //CREATE MODEL MATRIX RESOURCE
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(glm::mat4x4) * MAX_MESHES);
    CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    modelMatricesRsc = std::make_unique<DXResource>(device, heapProperties, bufferDesc, nullptr, "Model matrix structured buffer");
    modelMatricesRsc->CreateUploadBuffer(device, sizeof(glm::mat4x4) * MAX_MESHES, 0);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.NumElements = MAX_MESHES;
    srvDesc.Buffer.StructureByteStride = sizeof(glm::mat4x4);
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    device->CreateShaderResourceView(modelMatricesRsc->Get(), &srvDesc, engineDevice.GetDescriptorHeap(RESOURCE_HEAP)->GetCPUHandle(MODEL_MATRIX_RSC));

    //CLOSE COMMAND LIST
    engineDevice.EndFrame();
}

void Engine::Renderer::Render(const World& world)
{
}

Engine::MetaType Engine::Renderer::Reflect()
{
    return MetaType{ MetaType::T<Renderer>{}, "Renderer", MetaType::Base<System>{} };
}
