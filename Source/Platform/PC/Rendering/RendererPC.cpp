#include "Precomp.h"
#include "Platform/PC/Rendering/RendererPC.h"
#include "Core/Device.h"
#include "Core/FileIO.h"

#include "Platform/PC/Rendering/DX12Classes/DXSignature.h"
#include "Platform/PC/Rendering/DX12Classes/DXConstBuffer.h"
#include "Platform/PC/Rendering/DX12Classes/DXPipeline.h"
#include "Platform/PC/Rendering/DX12Classes/DXDescHeap.h"
#include "Platform/PC/Rendering/DX12Classes/DXResource.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TransformComponent.h"
#include "World/Registry.h"
#include "World/World.h"
#include "Meta/MetaType.h"
#include "Meta/MetaManager.h"

#include "World/WorldRenderer.h"
#include "Components/CameraComponent.h"
#include "Assets/Material.h"
#include "Assets/Texture.h"
#include "Assets/StaticMesh.h"

Engine::Renderer::Renderer()
{
    Device& engineDevice = Device::Get();
    ID3D12Device5* device = reinterpret_cast<ID3D12Device5*>(engineDevice.GetDevice());

    //CREATE ROOT SIGNATURE
    mSignature = std::make_unique<DXSignature>(12);
    mSignature->AddCBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);//0
    mSignature->AddCBuffer(1, D3D12_SHADER_VISIBILITY_PIXEL);//1
    mSignature->AddCBuffer(2, D3D12_SHADER_VISIBILITY_VERTEX);//2
    mSignature->AddCBuffer(3, D3D12_SHADER_VISIBILITY_PIXEL);//3
    mSignature->AddCBuffer(4, D3D12_SHADER_VISIBILITY_PIXEL);//4

    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);//5
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);//6
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);//7
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);//8
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);//9

    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 1);//10
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 2);//11
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 3);//12
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 4);//13

    mSignature->AddTable(D3D12_SHADER_VISIBILITY_VERTEX, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5);//14
    mSignature->AddTable(D3D12_SHADER_VISIBILITY_PIXEL, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);//15
    mSignature->AddSampler(0, D3D12_SHADER_VISIBILITY_PIXEL, D3D12_TEXTURE_ADDRESS_MODE_WRAP);//16
    mSignature->CreateSignature(device, L"MAIN ROOT SIGNATURE");

    //CREATE PBR PIPELINE
    FileIO& fileIO = FileIO::Get();
    std::string shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRVertex.hlsl");
    ComPtr<ID3DBlob> v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/PBRPixel.hlsl");
    ComPtr<ID3DBlob> p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
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

    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/SkyboxVertex.hlsl");
    v = DXPipeline::ShaderToBlob(shaderPath.c_str(), "vs_5_0");
    shaderPath = fileIO.GetPath(FileIO::Directory::EngineAssets, "shaders/HLSL/SkyboxPixel.hlsl");
    p = DXPipeline::ShaderToBlob(shaderPath.c_str(), "ps_5_0", "main");
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
    mConstBuffers[MODEL_MATRIX_CB] = std::make_unique<DXConstBuffer>(device, sizeof(glm::mat4x4), MAX_MESHES, "Mesh matrix data", FRAME_BUFFER_COUNT);
}

void Engine::Renderer::Render(const World& world)
{
    Device& engineDevice = Device::Get();
    ID3D12GraphicsCommandList4* commandList = reinterpret_cast<ID3D12GraphicsCommandList4*>(engineDevice.GetCommandList());
    std::shared_ptr<DXDescHeap> resourceHeap = engineDevice.GetDescriptorHeap(RESOURCE_HEAP);
    int frameIndex = engineDevice.GetFrameIndex();

    //GET WORLD
    const auto optionalEntityCameraPair = world.GetRenderer().GetMainCamera();
    ASSERT_LOG(optionalEntityCameraPair.has_value(), "DX12 draw requests have been made, but they cannot be cleared as there is no camera to draw them to");

    //UPDATE CAMERA
    const auto camera = optionalEntityCameraPair->second;
    InfoStruct::DXMatrixInfo matrixInfo;
    //matrixInfo.pm = camera.GetProjection();
    matrixInfo.pm = glm::transpose(glm::perspectiveLH(45.0f * (3.14f / 180.0f), 1.77f, 0.5f, 1000.f));
    matrixInfo.vm = glm::transpose(camera.GetView());
    matrixInfo.ipm = glm::inverse(matrixInfo.pm);
    matrixInfo.ivm = glm::inverse(matrixInfo.vm);
    mConstBuffers[CAM_MATRIX_CB]->Update(&matrixInfo, sizeof(InfoStruct::DXMatrixInfo), 0, frameIndex);

    //UPDATE LIGHTS
    lights.dirLights[0].colorAndIntensity = glm::vec4{ 1.0f };
    lights.dirLights[0].dir = normalize(glm::vec4{ .5f, -.5f, 1.0f, 0.f });
    mConstBuffers[LIGHT_CB]->Update(&lights, sizeof(InfoStruct::DXLightInfo), 0, frameIndex);

    commandList->SetGraphicsRootSignature(mSignature->GetSignature().Get());
    commandList->SetPipelineState(mPipelines[PBR_PIPELINE]->GetPipeline().Get());

    //BIND CONSTANT BUFFERS
    mConstBuffers[LIGHT_CB]->Bind(commandList, 1, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 0, 0, frameIndex);
    mConstBuffers[CAM_MATRIX_CB]->Bind(commandList, 4, 0, frameIndex);

    ID3D12DescriptorHeap* descriptorHeaps[] = {resourceHeap->Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    const auto view = world.GetRegistry().View<const StaticMeshComponent, const TransformComponent>();
    int meshCounter = 0;

    for (auto [entity, staticMeshComponent, transform] : view.each())
    {
        //UPDATE AND BIND MODEL MATRIX
        glm::mat4x4 modelMatrix = glm::transpose(transform.GetWorldMatrix());
        mConstBuffers[MODEL_MATRIX_CB]->Update(&modelMatrix, sizeof(glm::mat4x4), meshCounter, frameIndex);
        mConstBuffers[MODEL_MATRIX_CB]->Bind(commandList, 2, meshCounter, frameIndex);

        //UPDATE AND BIND MATERIAL INFO
        InfoStruct::DXMaterialInfo materialInfo;
        if (staticMeshComponent.mMaterial != nullptr) {
            materialInfo.colorFactor = { staticMeshComponent.mMaterial->mBaseColorFactor.r,
                staticMeshComponent.mMaterial->mBaseColorFactor.g,
                staticMeshComponent.mMaterial->mBaseColorFactor.b,
                0.f };
            materialInfo.emissiveFactor = { staticMeshComponent.mMaterial->mEmissiveFactor.r,
                staticMeshComponent.mMaterial->mEmissiveFactor.g,
                staticMeshComponent.mMaterial->mEmissiveFactor.b,
                0.f };
            materialInfo.metallicFactor = staticMeshComponent.mMaterial->mMetallicFactor;
            materialInfo.roughnessFactor = staticMeshComponent.mMaterial->mRoughnessFactor;
            materialInfo.normalScale = staticMeshComponent.mMaterial->mNormalScale;
            materialInfo.useColorTex = staticMeshComponent.mMaterial->mBaseColorTexture != nullptr;
            materialInfo.useEmissiveTex = staticMeshComponent.mMaterial->mEmissiveTexture != nullptr;
            materialInfo.useMetallicRoughnessTex = staticMeshComponent.mMaterial->mMetallicRoughnessTexture != nullptr;
            materialInfo.useNormalTex = staticMeshComponent.mMaterial->mNormalTexture != nullptr;
            materialInfo.useOcclusionTex = staticMeshComponent.mMaterial->mOcclusionTexture != nullptr;
            
            //BIND TEXTURES
            if (materialInfo.useColorTex)
            {
                resourceHeap->BindToGraphics(commandList, 5, staticMeshComponent.mMaterial->mBaseColorTexture->GetIndex());
            }
            if (materialInfo.useEmissiveTex)
            {
                resourceHeap->BindToGraphics(commandList, 6, staticMeshComponent.mMaterial->mEmissiveTexture->GetIndex());
            }
            if (materialInfo.useMetallicRoughnessTex)
            {
                resourceHeap->BindToGraphics(commandList, 7, staticMeshComponent.mMaterial->mMetallicRoughnessTexture->GetIndex());
            }
            if (materialInfo.useNormalTex)
            {
                resourceHeap->BindToGraphics(commandList, 8, staticMeshComponent.mMaterial->mNormalTexture->GetIndex());
            }
            if (materialInfo.useOcclusionTex)
            {
                resourceHeap->BindToGraphics(commandList, 9, staticMeshComponent.mMaterial->mOcclusionTexture->GetIndex());
            }
        }
        else {
            materialInfo.colorFactor = {0.f, 0.f, 0.f, 0.f };
            materialInfo.emissiveFactor = {0.f, 0.f, 0.f, 0.f };
            materialInfo.metallicFactor = 0.f;
            materialInfo.roughnessFactor = 0.f;
            materialInfo.normalScale = 0.f;
            materialInfo.useColorTex = false;
            materialInfo.useEmissiveTex = false;
            materialInfo.useMetallicRoughnessTex = false;
            materialInfo.useNormalTex = false;
            materialInfo.useOcclusionTex = false;

        }
        mConstBuffers[MATERIAL_CB]->Update(&materialInfo, sizeof(InfoStruct::DXMaterialInfo), meshCounter, frameIndex);
        mConstBuffers[MATERIAL_CB]->Bind(commandList, 3, meshCounter, frameIndex);

        //DRAW THE MESH

        if (!staticMeshComponent.mStaticMesh)
        {
            continue;
        }

        staticMeshComponent.mStaticMesh->DrawMesh();

        meshCounter++;
    }
}

Engine::MetaType Engine::Renderer::Reflect()
{
    return MetaType{ MetaType::T<Renderer>{}, "Renderer", MetaType::Base<System>{} };
}
